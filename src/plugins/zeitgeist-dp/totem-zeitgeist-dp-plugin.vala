using Totem;

struct MediaInfo {
  int64 timestamp;
  bool sent_access;
  string? mrl;
  string? mimetype;
  string? title;
  string? interpretation;
  string? artist;
  string? album;
}

class ZeitgeistDpPlugin: GLib.Object, Peas.Activatable {
  private MediaInfo current_media;
  /* timer waiting while we get some info about current playing media */
  private uint media_info_timeout;
  /* timer making sure we don't wait indefinitely */
  private uint timeout_id;
  private ulong[] signals;

  private Zeitgeist.Log zg_log;
  private Zeitgeist.DataSourceRegistry zg_registry;

  public GLib.Object object { owned get; construct; }

  public void activate () {
    Totem.Object totem = (Totem.Object) this.object;

    zg_log = new Zeitgeist.Log ();
    zg_registry = new Zeitgeist.DataSourceRegistry ();

    current_media = MediaInfo ();

    signals += Signal.connect_swapped (totem, "file-has-played",
                                       (Callback) file_has_played, this);
    signals += Signal.connect_swapped (totem, "file-closed",
                                       (Callback)file_closed, this);
    signals += Signal.connect_swapped (totem, "metadata-updated",
                                       (Callback) metadata_changed, this);
    signals += Signal.connect_swapped (totem, "notify::playing",
                                       (Callback) playing_changed, this);

    GenericArray<Zeitgeist.Event> templates =
      new GenericArray<Zeitgeist.Event> ();
    var event = new Zeitgeist.Event.full ("", Zeitgeist.ZG.USER_ACTIVITY,
                                          "application://totem.desktop", null);
    templates.add (event);
    var ds = new Zeitgeist.DataSource.full (
      "org.gnome.Totem,dataprovider",
      "Totem dataprovider",
      "Logs access/leave events for media files played with Totem",
      templates
    );
    zg_registry.register_data_source.begin (ds, null);
  }

  public void deactivate () {
    Totem.Object totem = (Totem.Object) this.object;

    /* we don't always get file-closed, so lets simulate it */
    file_closed (totem);

    foreach (ulong id in signals) {
      SignalHandler.disconnect (totem, id);
    }
    signals = null;

    /* cleanup timers */
    if (media_info_timeout != 0) Source.remove (media_info_timeout);
    if (timeout_id != 0) Source.remove (timeout_id);

    media_info_timeout = 0;
    timeout_id = 0;
  }

  public void update_state () {
    /* ignore */
  }

  private void restart_watcher (uint interval) {
    if (timeout_id != 0) {
      Source.remove (timeout_id);
    }
    timeout_id = Timeout.add (interval, timeout_cb);
  }

  private void file_has_played (string mrl, Totem.Object totem) {
    if (current_media.mrl != null)
      file_closed (totem);

    current_media = MediaInfo ();
    current_media.mrl = mrl;

    TimeVal cur_time = TimeVal ();
    current_media.timestamp = Zeitgeist.Timestamp.from_timeval (cur_time);

    /* wait a bit for the media info */
    if (media_info_timeout == 0) {
      media_info_timeout = Timeout.add (250, wait_for_media_info);
      /* but make sure we dont wait indefinitely */
      restart_watcher (15000);
    }
  }

  private void file_closed (Totem.Object totem) {
    if (current_media.sent_access && current_media.mrl != null) {
      /* send close event */
      TimeVal cur_time = TimeVal ();
      current_media.timestamp = Zeitgeist.Timestamp.from_timeval (cur_time);
      send_event_to_zg (true);

      current_media.mrl = null;
    }

    /* kill timers */
    if (media_info_timeout != 0) Source.remove (media_info_timeout);
    media_info_timeout = 0;
    if (timeout_id != 0) Source.remove (timeout_id);
    timeout_id = 0;
  }

  private void metadata_changed (string? artist, string? title, string? album,
                                 uint track_num, Totem.Object totem) {
    /* we can get some notification after sending event to ZG, so ignore it */
    if (media_info_timeout != 0) {
      current_media.artist = artist;
      current_media.title = title;
      current_media.album = album;
    }
  }

  private bool timeout_cb () {
    Totem.Object totem = (Totem.Object) this.object;

    if (media_info_timeout != 0) {
      /* we don't have any info besides the url, so use the short_title */

      Source.remove (media_info_timeout);
      media_info_timeout = 0;

      current_media.title = Totem.get_short_title (totem);
      timeout_id = 0;
      wait_for_media_info ();
    }

    timeout_id = 0;
    return false;
  }

  private async void query_media_mimetype (string current_mrl) {
    Totem.Object totem = (Totem.Object) this.object;
    string mrl = current_mrl;
    var f = File.new_for_uri (mrl);

    try {
      var fi = yield f.query_info_async (FileAttribute.STANDARD_CONTENT_TYPE,
                                         0, Priority.DEFAULT_IDLE, null);

      if (current_media.mrl != mrl || !totem.is_playing ()) return;
      current_media.mimetype = fi.get_content_type ();

      /* send event */
      send_event_to_zg ();
      current_media.sent_access = true;
    } catch (GLib.Error err) {
      /* most likely invalid uri */
    }
  }

  private bool wait_for_media_info () {
    Totem.Object totem = (Totem.Object) this.object;

    if (current_media.title != null && totem.is_playing ()) {
      Value val;
      var video = totem.get_video_widget () as Bacon.VideoWidget;
      video.get_metadata (Bacon.MetadataType.HAS_VIDEO, out val);
      current_media.interpretation = val.get_boolean () ?
        Zeitgeist.NFO.VIDEO : Zeitgeist.NFO.AUDIO;

      query_media_mimetype (current_media.mrl);

      /* cleanup timers */
      if (timeout_id != 0) Source.remove (timeout_id);
      timeout_id = 0;
      media_info_timeout = 0;
      return false;
    }
    /* wait longer */
    return true;
  }

  private void playing_changed () {
    Totem.Object totem = (Totem.Object) this.object;

    if (media_info_timeout == 0 && current_media.sent_access == false) {
      wait_for_media_info ();
    }

    /* end of playlist */
    if (!totem.is_playing () && current_media.sent_access) {
      /* sends leave event even if the user just pauses the playback
         for a little while, but we don't want too many access events
         for the same uri */
      file_closed (totem);
    }
  }

  private void send_event_to_zg (bool leave_event = false) {
    if (current_media.mrl != null && current_media.title != null) {
      string event_interpretation = leave_event ?
        Zeitgeist.ZG.LEAVE_EVENT : Zeitgeist.ZG.ACCESS_EVENT;
      string origin = Path.get_dirname (current_media.mrl);
      var subject = new Zeitgeist.Subject.full (
        current_media.mrl,
        current_media.interpretation,
        Zeitgeist.manifestation_for_uri (current_media.mrl),
        current_media.mimetype,
        origin,
        current_media.title,
        "");
      GenericArray<Zeitgeist.Event> events =
        new GenericArray<Zeitgeist.Event> ();
      var event = new Zeitgeist.Event.full (event_interpretation,
                                            Zeitgeist.ZG.USER_ACTIVITY,
                                            "application://totem.desktop",
                                            null);
      event.add_subject (subject);
      events.add (event);
      event.timestamp = current_media.timestamp;
      zg_log.insert_events_no_reply (events);
    }
  }
}

[ModuleInit]
public void peas_register_types (GLib.TypeModule module)
{
  var objmodule = module as Peas.ObjectModule;
  objmodule.register_extension_type (typeof (Peas.Activatable),
                                     typeof (ZeitgeistDpPlugin));
}

