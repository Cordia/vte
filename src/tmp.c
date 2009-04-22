#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#define TEST_STRING "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789~!@#$%^&*()_+=-[{]}\\|:;'\",<.>/?"
#define THE_FONT "Courier 24"

static GdkColor clr_white = {.pixel = 0, .red = 0xffff, .green = 0xffff, .blue = 0xffff};
static GString *str = NULL;
static PangoLayout *layout = NULL;

static int cx_font = 0, cy_font = 0, x_cursor = 0, y_cursor = 0, n_rows = 0, n_cols = 0;

static void
calc_sizes(GdkDrawable *window, gint *p_cx_window, gint *p_cy_window)
{
  gint new_n_rows, new_n_cols, cx, cy;

  if (!p_cx_window) p_cx_window = &cx;
  if (!p_cy_window) p_cx_window = &cy;

  gdk_drawable_get_size(window, p_cx_window, p_cy_window);
  new_n_rows = ((int)(((double)(*p_cy_window)) /  ((double)(cy_font))));
  new_n_cols = ((int)(((double)(*p_cx_window)) /  ((double)(cx_font))));
  if (!(new_n_rows == n_rows && new_n_cols == n_cols)) {
    g_string_printf(str, "%*s", n_rows * n_cols, " ");
    n_rows = new_n_rows;
    n_cols = new_n_cols;
  }
}

static void
real_commit(char *txt, GtkWidget *wnd)
{
  int len = strlen(txt);

  if (len) {
    g_string_overwrite(str, CLAMP(x_cursor, 0, n_cols - 1) + CLAMP(y_cursor, 0, n_rows - 1) * n_cols, txt);
    gtk_widget_queue_draw(wnd);
    x_cursor += len;
    y_cursor += x_cursor / n_cols;
    x_cursor = x_cursor % n_cols;
    if (x_cursor >= n_cols - 1 && y_cursor >= n_rows - 1) {
      x_cursor = 0;
      y_cursor = 0;
    }
  }
}

static void
imc_commit(GtkIMContext *imc, gchar *txt, GtkWidget *wnd)
{
  g_print("commit \"%s\"\n", txt);
  real_commit(txt, wnd);
}

static gboolean
unfiltered_key_press_event(GtkWidget *wnd, GdkEvent *event, GtkIMContext *imc)
{
  switch(event->key.keyval) {

    case GDK_BackSpace:
      x_cursor--;
      if (x_cursor < 0) {
        if (y_cursor > 0) {
          x_cursor = n_cols - 1;
          y_cursor--;
        }
        else
          x_cursor = 0;
      }
      return TRUE;

    default:
      g_print("Key press gives \"%s\"\n", event->key.string);
      real_commit(event->key.string, wnd);
      return TRUE;
  }
}

static gboolean
filter_event(GtkWidget *wnd, GdkEvent *event, GtkIMContext *imc)
{
  g_print("filter_event\n");
  if (!hildon_gtk_im_context_filter_event(imc, event)) {
    switch (event->type) {

      case GDK_KEY_PRESS:
        if (!gtk_im_context_filter_keypress(imc, &(event->key)))
          return unfiltered_key_press_event(wnd, event, imc);
        else
          g_print("IM context handles this key press\n");
        return TRUE;

      case GDK_KEY_RELEASE:
        if (!gtk_im_context_filter_keypress(imc, &(event->key)))
          return FALSE;
        else
          g_print("IM context handles this key release\n");
        return TRUE;

      default:
        return FALSE;
    }
  }

  return TRUE;
}

static GtkIMContext *
maybe_create_imc(GtkWidget *widget, char *msg) {
  GtkIMContext *imc = g_object_get_data(G_OBJECT(widget), "im-context");

  if (!imc) {
    if (widget->window) {
      g_print("create_imc: %s Creating imc\n", msg);
      imc = gtk_im_multicontext_new();
      gtk_im_context_set_client_window(imc, widget->window);
      g_object_set(G_OBJECT(imc), "hildon-input-mode", (HILDON_GTK_INPUT_MODE_FULL | HILDON_GTK_INPUT_MODE_MULTILINE) & (~HILDON_GTK_INPUT_MODE_AUTOCAP), NULL);
      g_object_set_data_full(G_OBJECT(widget), "im-context", imc, (GDestroyNotify)g_object_unref);
      g_signal_connect(G_OBJECT(imc), "commit", (GCallback)imc_commit, widget);
      g_signal_connect(G_OBJECT(widget), "key-press-event", (GCallback)filter_event, imc);
      g_signal_connect(G_OBJECT(widget), "key-release-event", (GCallback)filter_event, imc);
      g_signal_connect(G_OBJECT(widget), "button-press-event", (GCallback)filter_event, imc);
      g_signal_connect(G_OBJECT(widget), "button-release-event", (GCallback)filter_event, imc);
      g_signal_connect_swapped(G_OBJECT(widget), "focus-in-event", (GCallback)gtk_im_context_focus_in, imc);
      g_signal_connect_swapped(G_OBJECT(widget), "focus-out-event", (GCallback)gtk_im_context_focus_out, imc);
    }
    else
      g_print("create_imc: %s widget->window is NULL, so not creating imc\n", msg);
  }
  else
      g_print("create_imc: %s imc already created\n", msg);

  return imc;
}

static
gboolean expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer null)
{
  GdkGC *gc = NULL;
  int Nix, cx_window, cy_window;

  maybe_create_imc(widget, "From expose_event:");

  calc_sizes(widget->window, &cx_window, &cy_window);

  gdk_window_clear(widget->window);

  gc = gdk_gc_new(widget->window);
  gdk_gc_set_foreground(gc, &clr_white);
  gdk_gc_set_background(gc, &clr_white);
  for (Nix = 0 ; Nix < n_rows && Nix * n_cols < strlen(str->str) ; Nix++) {
    pango_layout_set_text(layout, &((str->str)[Nix * n_cols]), n_cols);
    gdk_draw_layout(widget->window, gc, 0, Nix * cy_font, layout);
  }
  g_object_unref(gc);

  return TRUE;
}

static PangoLayout *
init_font_stuff()
{
  PangoContext *ctx = NULL;
  PangoFontDescription *font_desc = pango_font_description_from_string(THE_FONT);
  PangoLayout *layout = NULL;

  ctx = gdk_pango_context_get();
  pango_context_set_font_description(ctx, font_desc);
  layout = pango_layout_new(ctx);
  pango_layout_set_text(layout, TEST_STRING, -1);
  pango_layout_get_pixel_size(layout, &cx_font, &cy_font);
  cx_font /= strlen(TEST_STRING);
  pango_font_description_free(font_desc);
  g_object_unref(ctx);

  return layout;
}

int
main(int argc, char **argv)
{
  GtkWidget *wnd = NULL;

  gtk_init(&argc, &argv);

  layout = init_font_stuff();

  str = g_string_new("");
  gdk_rgb_find_color(gdk_colormap_get_system(), &clr_white);

  wnd = g_object_new(GTK_TYPE_WINDOW, NULL);
  GTK_WIDGET_SET_FLAGS(wnd, GTK_WIDGET_FLAGS(wnd) | GTK_DOUBLE_BUFFERED);
  gtk_widget_add_events(wnd, GDK_EXPOSURE_MASK | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  g_signal_connect(wnd, "delete-event", (GCallback)gtk_main_quit, NULL);
  g_signal_connect(wnd, "expose-event", (GCallback)expose_event, NULL);

  gtk_widget_show(wnd);

  maybe_create_imc(wnd, "From main:");

  gtk_main();

  g_string_free(str, TRUE);

  g_object_unref(layout);

  return 0;
}
