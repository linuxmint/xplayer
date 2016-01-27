using GLib;
using Totem;

class SampleValaPlugin: GLib.Object, Peas.Activatable {
	public GLib.Object object { owned get; construct; }

	public void activate () {
		print ("Hello world\n");
	}

	public void deactivate () {
		print ("Goodbye world\n");
	}

	/* Gir doesn't allow marking functions as non-abstract */
	public void update_state () {}
}

[ModuleInit]
public void peas_register_types (GLib.TypeModule module) {
	var objmodule = module as Peas.ObjectModule;
	objmodule.register_extension_type (typeof(Peas.Activatable), typeof(SampleValaPlugin));
}
