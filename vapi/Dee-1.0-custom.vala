namespace Dee {

  [CCode (type_id = "dee_model_get_type ()")]
  public interface Model {
    [CCode (array_length_pos = 1.33333, array_length_type = "guint", cname = "dee_model_build_named_row_sunk")]
    public GLib.Variant[] build_named_row ([CCode (array_length = false)] GLib.Variant[]? out_row_members, string first_column_name, ...);
    [CCode (array_length_pos = 1.33333, array_length_type = "guint", cname = "dee_model_build_named_row_sunk")]
    public unowned GLib.Variant[] build_named_row_static ([CCode (array_length = false)] GLib.Variant[] out_row_members, string first_column_name, ...);
    [CCode (array_length = false, array_null_terminated = true)]
    public abstract GLib.Variant[] get_row (Dee.ModelIter iter, [CCode (array_length = false)] GLib.Variant[]? out_row_members = null);
    [CCode (array_length = false, array_null_terminated = true, cname = "dee_model_get_row")]
    public unowned GLib.Variant[] get_row_static (Dee.ModelIter iter, [CCode (array_length = false)] GLib.Variant[] out_row_members);
    public void set_column_names (string first_column_name, ...);
  }

	public struct Filter {
		[CCode (cname = "destroy")]
		public GLib.DestroyNotify destroy_notify;
		public static Filter @new (Dee.StaticFilterMapFunc map_func, owned Dee.FilterMapNotify map_notify);
	}

	[CCode (ref_function = "", unref_function = "")]
	[Compact]
	public class ModelIter {
	}

	[CCode (cheader_filename = "dee.h", ref_function = "", unref_function = "")]
	[Compact]
	public class ModelTag<G> {
		[CCode (cname = "dee_model_register_tag", simple_generics = true)]
		public ModelTag (Dee.Model model);
		[CCode (cname = "dee_model_set_tag", instance_pos = 2.1)]
		public void @set (Dee.Model model, Dee.ModelIter iter, owned G value);
		[CCode (cname = "dee_model_get_tag", instance_pos = 2.1)]
		public unowned G @get (Dee.Model model, Dee.ModelIter iter);
		[CCode (cname = "dee_model_clear_tag", instance_pos = 2.1)]
		public void clear (Dee.Model model, Dee.ModelIter iter);
	}

	[CCode (type_id = "dee_result_set_get_type ()")]
	public interface ResultSet {
		[CCode (cname = "_vala_dee_result_set_iterator")]
		public Dee.ResultSet iterator ();
		[CCode (cname = "_vala_dee_result_set_next_value")]
		public unowned Dee.ModelIter? next_value ();
	}

	[CCode (type_id = "dee_serializable_get_type ()")]
	public interface Serializable : GLib.Object {
		[CCode (returns_floating_reference = true)]
		public GLib.Variant externalize ();
	}

	[CCode (type_id = "dee_term_list_get_type ()")]
	public class TermList : GLib.Object {
		[CCode (cname = "dee_term_list_get_term")]
		public unowned string @get (uint index);
		public uint size {
			[CCode (cname = "dee_term_list_num_terms")]
			get;
		}
	}

	[CCode (cheader_filename = "dee.h", has_target = false, cname = "DeeFilterMapFunc")]
	public delegate void StaticFilterMapFunc (Dee.Model orig_model, Dee.FilterModel filter_model, void* data);

	[CCode (cheader_filename = "dee-icu.h", free_function = "dee_icu_term_filter_destroy")]
	[Compact]
	public class ICUTermFilter {
		public ICUTermFilter (string system_id, string? rules) throws Dee.ICUError;
		public ICUTermFilter.ascii_folder ();
	}
}
