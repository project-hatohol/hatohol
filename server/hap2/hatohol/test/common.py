def assertNotRaises(func, *args, **kwargs):
    try:
        func(*args, **kwargs)
    except Exception:
        raise

def returnPrivObj(instance, obj_name, class_name=None):
    if class_name is None:
        class_name = instance.__class__.__name__
    return eval("instance._"+class_name+obj_name)
