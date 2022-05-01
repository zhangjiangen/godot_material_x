# config.py

def can_build(env, platform):
    return env["platform"] != "javascript" and env["platform"] != "linuxbsd"

def configure(env):
    pass
