class WeakRef
  def method_missing(m, *args, &b)
    target = self.__getobj__
    target.respond_to?(m) ? target.__send__(m, *args, &b) : super(m, *args, &b)
  end
end
