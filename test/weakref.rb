assert('WeakRef.new') do
  ref = Object.new
  def ref.test; :test end
  ref = WeakRef.new ref
  ref.test == :test
end

assert('WeakRef#weakref_alive?') do
  ref = Object.new
  ref = WeakRef.new ref
  GC.start
  not ref.weakref_alive?
end

assert('weakref of weakref') do
  ref = Object.new
  weakref = WeakRef.new ref
  begin
    weakref = WeakRef.new weakref
    false
  rescue ArgumentError
    true
  end
end

assert('WeakRef::RefError') do
  ref = Object.new
  def ref.test; :test end
  ref = WeakRef.new ref
  GC.start
  begin
    ref.test
    false
  rescue WeakRef::RefError
    true
  end
end
