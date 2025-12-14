## Process this file with automake to produce Makefile.in


if PLATFORM_WIN32
        AM_CPPFLAGS = -DPACKAGE_LOCALE_DIR=\""../$(DATADIRNAME)/locale"\"
else
        AM_CPPFLAGS = -DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\"
endif

AM_CPPFLAGS += -DPACKAGE_SRC_DIR=\""$(srcdir)"\"  \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\"       \
	$(ARDESIA_CFLAGS)

AM_CFLAGS =\
	 -Wall                                    \
	 -g -L$(libdir)

bin_PROGRAMS = ardesia

ardesia_SOURCES = \
	bar.c                                     \
	bar.h                                     \
	bar_callbacks.c                           \
	bar_callbacks.h                           \
	annotation_config.c                       \
	annotation_config.h                       \
	annotation_window_callbacks.c             \
	annotation_window_callbacks.h             \
	background_config.c                       \
	background_config.h                       \
	background_window.c                       \
	background_window.h                       \
        broken.c                                  \
	broken.h                                  \
        spline.c                                  \
	spline.h                                  \
        crash_dialog.c                            \
	cairo_functions.c                         \
	cairo_functions.h                         \
	crash_dialog.h                            \
	crash_dialog_callbacks.c                  \
	crash_dialog_callbacks.h                  \
	config_path.c                             \
	crash_path.h                              \
	cursors.c                                 \
	cursors.h                                 \
        pdf_saver.c                               \
	pdf_saver.h                               \
        font_config.c                             \
	font_config.h                             \
        fill.c                                    \
	fill.h                                    \
        annotation_window.c                       \
	annotation_window.h                       \
        utils.c                                   \
	utils.h                                   \
        input.c                                   \
	input.h                                   \
        windows_utils.c                           \
	windows_utils.h                           \
	keyboard.c                                \
	keyboard.h                                \
        saver.c                                   \
	saver.h                                   \
        color_selector.c                          \
	color_selector.h                          \
	font_selector.c                           \
	font_selector.h                           \
	font_selector_callbacks.c                 \
	font_selector_callbacks.h                 \
	iwb_saver.c                               \
	iwb_saver.h                               \
	iwb_loader.c                              \
	iwb_loader.h                              \
        preference_dialog.c                       \
	preference_dialog.h                       \
	preference_dialog_callbacks.c             \
	preference_dialog_callbacks.h             \
        project_dialog.c                          \
	project_dialog.h                          \
	project_dialog_callbacks.c                \
	project_dialog_callbacks.h                \
        share_confirmation_dialog.c               \
	share_confirmation_dialog.h               \
	share_confirmation_dialog_callbacks.c     \
	share_confirmation_dialog_callbacks.h     \
        text_input.c                              \
	text_input.h                              \
        user_config.c                             \
	user_config.h                             \
        info_dialog.c                             \
	info_dialog.h                             \
        recorder.c                                \
	recorder.h                                \
	workspace.c                               \
	monitor.c                                 \
	commandline.c                             \
	ardesia.c                                 \
	ardesia.h                                 \
	recorder_timer.c                          \
	recorder_timer.h                          \
	recordingstudio.c                         \
	recordingstudio.h                         \
	recordingstudio_callbacks.c               \
	recordingstudio_callbacks.h


ardesia_LDFLAGS =

if !PLATFORM_WIN32
if WITH_GNU_LD
ardesia_LDFLAGS += -Wl,--export-dynamic
endif
else
#This create a windows executable instead an msdos ones
ardesia_LDFLAGS += -mwindows -lbfd -lintl -liberty -limagehlp -lole32 -luuid
endif

ardesia_LDADD = $(ARDESIA_LIBS)
