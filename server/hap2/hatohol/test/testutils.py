def assertNotRaises(func, *args, **kwargs):
    try:
        func(*args, **kwargs)
    except Exception:
        raise


def get_mangled_name(instance, attr_name, class_name=None):
    if class_name is None:
        class_name = instance.__class__.__name__
    return "instance._" + class_name + attr_name


def get_priv_attr(instance, attr_name, class_name=None):
    return eval(get_mangled_name(instance, attr_name, class_name))


def set_priv_attr(instance, attr_name, new_attr, class_name=None):
    exec(get_mangled_name(instance, attr_name, class_name) + " = new_attr")
