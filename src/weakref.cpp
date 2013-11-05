#include <mruby.h>
#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/variable.h>

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>


namespace {

typedef boost::weak_ptr<RObject> weakref;
typedef boost::shared_ptr<RObject> strongref;

struct null_deleter {
  void operator()(RObject* obj) {}
};

void weakref_free(mrb_state* M, void* ptr) {
  reinterpret_cast<weakref*>(ptr)->~weakref();
  mrb_free(M, ptr);
}

void strongref_free(mrb_state* M, void* ptr) {
  reinterpret_cast<strongref*>(ptr)->~strongref();
  mrb_free(M, ptr);
}

mrb_data_type const weakref_type = { "WeakRef", &weakref_free };
mrb_data_type const strongref_type = { "StrongkRef", &strongref_free };

/*
 * returns strong reference of object
 * creates strong reference if it doesn't exist in IV '@__strongref'
 */
strongref& get_strongref(mrb_state* M, mrb_value const& v) {
  if(mrb_obj_is_kind_of(M, v, mrb_class_get(M, "WeakRef"))) {
    mrb_raise(M, mrb_class_get(M, "ArgumentError"), "cannot create weakref of weakref");
  }
  
  mrb_sym const sym = mrb_intern_cstr(M, "@__strongref");
  if(mrb_iv_defined(M, v, sym)) {
    return *reinterpret_cast<strongref*>(
        mrb_data_check_get_ptr(M, mrb_iv_get(M, v, sym), &strongref_type));
  } else {
    strongref* const ref = new(mrb_malloc(M, sizeof(strongref)))
                           strongref(mrb_obj_ptr(v), null_deleter());
    mrb_iv_set(M, v, sym, mrb_obj_value(
        mrb_data_object_alloc(M, M->object_class, ref, &strongref_type)));
    return *ref;
  }
}

weakref& get_weakref(mrb_state* M, mrb_value const& v) {
  return *reinterpret_cast<weakref*>(mrb_data_check_get_ptr(M, v, &weakref_type));
}

mrb_value weakref_init(mrb_state* M, mrb_value const self) {
  mrb_value src;
  mrb_get_args(M, "o", &src);
  strongref& ref = get_strongref(M, src);
  DATA_PTR(self) = new(mrb_malloc(M, sizeof(weakref))) weakref(ref);
  DATA_TYPE(self) = &weakref_type;
  return self;
}

mrb_value weakref_is_alive(mrb_state* M, mrb_value const self) {
  weakref& ref = get_weakref(M, self);
  if(ref.expired()) { ref.reset(); } // clear weak_ptr
  return mrb_bool_value(not ref.expired());
}

mrb_value weakref_getobj(mrb_state* M, mrb_value const self) {
  weakref& ref = get_weakref(M, self);
  if(ref.expired()) {
    ref.reset(); // clear weak_ptr
    mrb_raise(M, mrb_class_get_under(M, mrb_class_get(M, "WeakRef"), "RefError"),
              "invalid reference - already expired");
  }
  return mrb_obj_value(ref.lock().get());
}

}

extern "C" void mrb_mruby_weakref_gem_init(mrb_state* M) {
  RClass* const WeakRef = mrb_define_class(M, "WeakRef", mrb_class_get(M, "BasicObject"));
  mrb_define_class_under(M, WeakRef, "RefError", mrb_class_get(M, "StandardError"));

  MRB_SET_INSTANCE_TT(WeakRef, MRB_TT_DATA);

  mrb_define_method(M, WeakRef, "initialize", &weakref_init, MRB_ARGS_REQ(1));
  mrb_define_method(M, WeakRef, "weakref_alive?", &weakref_is_alive, MRB_ARGS_NONE());
  mrb_define_method(M, WeakRef, "__getobj__", &weakref_getobj, MRB_ARGS_NONE());
}

extern "C" void mrb_mruby_weakref_gem_final(mrb_state*) {}

