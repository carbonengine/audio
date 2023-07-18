def run_in_tasklet(func):
    def wrapped(*args, **kwargs):
        import stackless
        stackless.tasklet(func)(*args, **kwargs)
        assert(stackless.runcount == 2)
        stackless.run()
        if (stackless.runcount != 1):
            raise RuntimeError("Leaking tasklets")
    return wrapped