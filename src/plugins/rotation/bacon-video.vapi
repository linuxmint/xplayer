[CCode (cprefix = "Bacon", lower_case_cprefix = "bacon_")]

namespace Bacon {
	[CCode (cheader_filename = "bacon-video-widget.h")]
	public class VideoWidget : Gtk.Widget {
    [CCode (has_construct_function = false)]
		public VideoWidget () throws GLib.Error;

    public Rotation get_rotation ();
    public void set_rotation (Rotation rotation);
	}
  [CCode (cname="BvwRotation", cprefix = "BVW_ROTATION_", cheader_filename = "bacon-video-widget.h")]
  public enum Rotation {
    R_ZERO = 0,
    R_90R  = 1,
    R_180  = 2,
    R_90L  = 3
  }
}
