# From code by James Livingston

from gi.repository import GObject, Peas, Totem # pylint: disable-msg=E0611,W0611

class SamplePython (GObject.Object, Peas.Activatable):
    __gtype_name__ = 'SamplePython'

    object = GObject.property (type = GObject.Object)

    def __init__ (self):
        GObject.Object.__init__ (self)

    def do_activate (self):
        print "Activating sample Python plugin"
        self.object.action_fullscreen_toggle ()

    def do_deactivate (self):
        print "Deactivating sample Python plugin"
        self.object.action_fullscreen_toggle ()
