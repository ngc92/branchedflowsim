from ..cmdline_helpers import CmdLineBuilder


class Observer(CmdLineBuilder):
    """
    Base class for all Observer specifications. Observers require a static class variable
    `_MONODROMY_` set to either True of False to specify whether the observer requires
    monodromy tracing.
    """
    def __new__(cls, *args, **kwargs):
        if not hasattr(cls, "_MONODROMY_"):
            raise TypeError("No _MONODROMY_ found in %s (%s)" % (cls.__name__, cls))
        return super(Observer, cls).__new__(cls, *args, **kwargs)

    @property
    def need_monodromy(self):
        return getattr(self, "_MONODROMY_")

