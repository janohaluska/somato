/*
 * Copyright (c) 2004-2006  Daniel Elstner  <daniel.kitta@gmail.com>
 *
 * This file is part of Somato.
 *
 * Somato is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Somato is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Somato; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef SOMATO_GLSCENE_H_INCLUDED
#define SOMATO_GLSCENE_H_INCLUDED

#include <glibmm/ustring.h>
#include <pangomm/context.h>
#include <pangomm/layout.h>
#include <gtkmm/drawingarea.h>
#include <memory>
#include <vector>

#include <config.h>

namespace GL
{

struct Extensions;
class  LayoutTexture;

struct UIVertex
{
  float texcoord[2];
  float vertex[2];

  inline void set_texcoord(float s, float t) { texcoord[0] = s; texcoord[1] = t; }
  inline void set_vertex(float x, float y) { vertex[0] = x; vertex[1] = y; }

  inline UIVertex()
    { texcoord[0] = 0.0; texcoord[1] = 0.0; vertex[0] = 0.0; vertex[1] = 0.0; }

  inline UIVertex(const UIVertex& b)
    { set_texcoord(b.texcoord[0], b.texcoord[1]); set_vertex(b.vertex[0], b.vertex[1]); }

  inline UIVertex& operator=(const UIVertex& b)
    { set_texcoord(b.texcoord[0], b.texcoord[1]); set_vertex(b.vertex[0], b.vertex[1]); return *this; }
};

#if SOMATO_USE_UNCHECKEDVECTOR
typedef Util::UncheckedVector<LayoutTexture*> LayoutVector;
typedef Util::UncheckedVector<UIVertex>       GeometryVector;
#else
typedef std::vector<LayoutTexture*>           LayoutVector;
typedef std::vector<UIVertex>                 GeometryVector;
#endif

/*
 * Base GL widget class that implements all the generic stuff not specific
 * to the animation of Soma cubes.  Note that per convention all methods with
 * "gl_" prefix expect the caller to set up the GL context.  Also, be careful
 * to never invoke unknown functions or signal handlers while a GL context is
 * active, as recursive activation is not allowed.
 */
class Scene : public Gtk::DrawingArea
{
public:
  virtual ~Scene();

  void reset_counters();
  unsigned int get_frame_counter() const;
  unsigned int get_triangle_counter() const;

  void set_exclusive_context(bool exclusive_context);
  bool get_exclusive_context() const;

  void set_use_back_buffer(bool use_back_buffer);
  bool get_use_back_buffer() const;

  void set_enable_vsync(bool enable_vsync);
  bool get_enable_vsync() const;
  bool vsync_enabled() const;

  void set_show_focus(bool show_focus);
  bool get_show_focus() const;

protected:
  class ScopeContext;

  Scene();

  inline const GL::Extensions* gl_ext() const;

  LayoutTexture* create_layout_texture();
  void gl_update_ui();
  void gl_swap_buffers();

  virtual void gl_initialize();
  virtual void gl_cleanup();
  virtual void gl_reset_state();
  virtual int  gl_render() = 0;
  virtual void gl_update_viewport();
  virtual void gl_update_projection();
  virtual void gl_update_color();

  virtual void on_screen_changed(const Glib::RefPtr<Gdk::Screen>& previous_screen);
  virtual void on_size_allocate(Gtk::Allocation& allocation);
  virtual void on_state_changed(Gtk::StateType previous_state);
  virtual void on_style_changed(const Glib::RefPtr<Gtk::Style>& previous_style);
  virtual void on_direction_changed(Gtk::TextDirection previous_direction);
  virtual bool on_expose_event(GdkEventExpose* event);
  virtual bool on_focus_in_event(GdkEventFocus* event);
  virtual bool on_focus_out_event(GdkEventFocus* event);
  virtual bool on_visibility_notify_event(GdkEventVisibility* event);

private:
  virtual void setup_gl_context();
  virtual GL::Extensions* gl_query_extensions();
  virtual void gl_reposition_layouts();

  void*                         gl_drawable_;
  std::auto_ptr<GL::Extensions> gl_extensions_;
  Glib::RefPtr<Pango::Context>  texture_context_;

  GeometryVector                ui_geometry_;
  LayoutVector                  ui_layouts_;

  unsigned int                  ui_buffer_;
  unsigned int                  stipple_texture_;
  unsigned int                  frame_counter_;
  unsigned int                  triangle_counter_;

  unsigned char                 focus_color_[3];

  bool                          exclusive_context_;
  bool                          has_back_buffer_;
  bool                          use_back_buffer_;
  bool                          enable_vsync_;
  bool                          vsync_enabled_;
  bool                          show_focus_;
  bool                          focus_drawable_;
  bool                          use_multitexture_;

  void on_signal_realize();
  void on_signal_unrealize();

  Glib::RefPtr<Pango::Layout> create_texture_pango_layout(const Glib::ustring& text);

  void gl_update_vsync_state();
  void gl_update_layouts();
  void gl_init_stipple_texture();

  void gl_build_focus();
  void gl_build_layouts();

  int  gl_render_ui(void* arrays) const;
  int  gl_render_focus() const;
  int  gl_render_layouts() const;
  int  gl_render_layouts_multitexture(unsigned int target, LayoutVector::const_iterator first) const;
  int  gl_render_layouts_multipass(unsigned int target, LayoutVector::const_iterator first) const;
};

/*
 * Instantiating a GL::ScopeContext object makes the specified
 * widget's GL context current for the duration of its scope.
 */
class Scene::ScopeContext
{
private:
  Scene& scene_;

  // All non-inline methods are static, in order to enable
  // the compiler to entirely optimize away the object instance.
  static void begin_(Scene& scene);
  static void end_(Scene& scene);

  // noncopyable
  ScopeContext(const ScopeContext&);
  ScopeContext& operator=(const ScopeContext&);

public:
  explicit inline ScopeContext(Scene& scene) : scene_ (scene) { begin_(scene_); }
  inline ~ScopeContext() { end_(scene_); }
};

} // namespace GL

#endif /* SOMATO_GLSCENE_H_INCLUDED */
