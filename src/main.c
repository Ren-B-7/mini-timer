#include <fcntl.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#include "include/minicli.h"

#define SECONDS_PER_MINUTE 60
#define SECONDS_PER_HOUR 3600
#define BUFFER_SIZE 16
#define BASE_TEN 10
#define DEFAULT_WINDOW_WIDTH 300
#define DEFAULT_WINDOW_HEIGHT 340
#define DEFAULT_SPACING 8
#define CARD_SPACING 6
#define LABEL_XALIGN 0.5F

typedef struct {
	int remaining;
	int initial;
	GtkWidget* label;
	GtkWidget* status_label;
	GtkWidget* entry;
	GtkWidget* start_btn;
	GtkWidget* stop_btn;
	GtkWidget* reset_btn;
	GtkWidget* window;
	guint timer_id;
	gboolean dark_mode;
} TimerState;

/* ── Terminal helpers ── */

static int kbhit(void)
{
	struct termios oldt;
	struct termios newt;
	int character;
	int oldf;
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= (tcflag_t) ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
	character = getchar();
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	fcntl(STDIN_FILENO, F_SETFL, oldf);
	if (character != EOF) {
		ungetc(character, stdin);
		return 1;
	}
	return 0;
}

/* ── GUI helpers ── */

static void update_display(TimerState* state)
{
	char buf[BUFFER_SIZE];
	snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
	 state->remaining / SECONDS_PER_HOUR,
	 (state->remaining % SECONDS_PER_HOUR) / SECONDS_PER_MINUTE,
	 state->remaining % SECONDS_PER_MINUTE);
	gtk_label_set_text(GTK_LABEL(state->label), buf);
}

static void set_status(TimerState* state, const char* text, gboolean running)
{
	gtk_label_set_text(GTK_LABEL(state->status_label), text);
	GtkStyleContext* ctx = gtk_widget_get_style_context(state->status_label);
	if (running) {
		gtk_style_context_add_class(ctx, "running");
	} else {
		gtk_style_context_remove_class(ctx, "running");
	}
}

/* ── Timer tick ── */

static gboolean handle_timer_tick(TimerState* state)
{
	if (state->remaining > 0) {
		state->remaining--;
		update_display(state);
		return TRUE;
	}
	gtk_label_set_text(GTK_LABEL(state->label), "00:00:00");
	set_status(state, "DONE", FALSE);
	state->timer_id = 0;
	return FALSE;
}

/* ── Button callbacks ── */

static void on_start_clicked(GtkWidget* widget, gpointer data)
{
	(void) widget;
	TimerState* state = (TimerState*) data;

	/* Stop any running timer */
	if (state->timer_id > 0) {
		g_source_remove(state->timer_id);
		state->timer_id = 0;
	}

	const char* text = gtk_entry_get_text(GTK_ENTRY(state->entry));
	if (text == NULL || text[0] == '\0') {
		return;
	}

	state->remaining = (int) strtol(text, NULL, BASE_TEN);
	state->initial = state->remaining;
	update_display(state);
	set_status(state, "RUNNING", TRUE);
	state->timer_id =
	 g_timeout_add_seconds(1, (GSourceFunc) handle_timer_tick, state);
}

static void on_stop_clicked(GtkWidget* widget, gpointer data)
{
	(void) widget;
	TimerState* state = (TimerState*) data;
	if (state->timer_id > 0) {
		g_source_remove(state->timer_id);
		state->timer_id = 0;
	}
	set_status(state, "PAUSED", FALSE);
}

static void on_reset_clicked(GtkWidget* widget, gpointer data)
{
	(void) widget;
	TimerState* state = (TimerState*) data;
	if (state->timer_id > 0) {
		g_source_remove(state->timer_id);
		state->timer_id = 0;
	}
	state->remaining = state->initial;
	update_display(state);
	set_status(state, "READY", FALSE);
}

static void on_theme_toggle_clicked(GtkWidget* widget, gpointer data)
{
	(void) widget;
	TimerState* state = (TimerState*) data;
	state->dark_mode = !state->dark_mode;

	GtkStyleContext* ctx = gtk_widget_get_style_context(state->window);
	GtkSettings* settings = gtk_settings_get_default();

	if (state->dark_mode) {
		gtk_style_context_add_class(ctx, "dark-mode");
		g_object_set(settings, "gtk-application-prefer-dark-theme", TRUE, NULL);
	} else {
		gtk_style_context_remove_class(ctx, "dark-mode");
		g_object_set(settings, "gtk-application-prefer-dark-theme", FALSE,
		 NULL);
	}
}

/* ── App activate ── */

static void activate(GtkApplication* app, gpointer user_data)
{
	TimerState* state = (TimerState*) user_data;

	/* Load stylesheet from gresource */
	GtkCssProvider* provider = gtk_css_provider_new();
	gtk_css_provider_load_from_resource(provider, "/org/mini/timer/style.css");
	gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
	 GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	g_object_unref(provider);

	/* Window */
	state->window = gtk_application_window_new(app);
	gtk_window_set_title(GTK_WINDOW(state->window), "Mini Timer");
	gtk_window_set_default_size(GTK_WINDOW(state->window), DEFAULT_WINDOW_WIDTH,
	 DEFAULT_WINDOW_HEIGHT);
	gtk_window_set_resizable(GTK_WINDOW(state->window), FALSE);

	/* Outer box */
	GtkWidget* outer = gtk_box_new(GTK_ORIENTATION_VERTICAL, DEFAULT_SPACING);
	gtk_style_context_add_class(gtk_widget_get_style_context(outer),
	 "timer-box");
	gtk_container_add(GTK_CONTAINER(state->window), outer);

	/* Top row: spacer + theme toggle */
	GtkWidget* top_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	GtkWidget* spacer = gtk_label_new("");
	gtk_widget_set_hexpand(spacer, TRUE);
	gtk_box_pack_start(GTK_BOX(top_row), spacer, TRUE, TRUE, 0);

	GtkWidget* theme_btn = gtk_button_new_with_label("◑  Theme");
	gtk_style_context_add_class(gtk_widget_get_style_context(theme_btn),
	 "theme-toggle");
	g_signal_connect(theme_btn, "clicked", G_CALLBACK(on_theme_toggle_clicked),
	 state);
	gtk_box_pack_end(GTK_BOX(top_row), theme_btn, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(outer), top_row, FALSE, FALSE, 0);

	/* Display card */
	GtkWidget* card = gtk_box_new(GTK_ORIENTATION_VERTICAL, CARD_SPACING);
	gtk_style_context_add_class(gtk_widget_get_style_context(card),
	 "display-card");
	gtk_box_pack_start(GTK_BOX(outer), card, FALSE, FALSE, 4);

	state->label = gtk_label_new("00:00:00");
	gtk_style_context_add_class(gtk_widget_get_style_context(state->label),
	 "timer-label");
	gtk_label_set_xalign(GTK_LABEL(state->label), LABEL_XALIGN);
	gtk_box_pack_start(GTK_BOX(card), state->label, FALSE, FALSE, 0);

	state->status_label = gtk_label_new("READY");
	gtk_style_context_add_class(
	 gtk_widget_get_style_context(state->status_label), "status-label");
	gtk_label_set_xalign(GTK_LABEL(state->status_label), LABEL_XALIGN);
	gtk_box_pack_start(GTK_BOX(card), state->status_label, FALSE, FALSE, 0);

	/* Entry */
	state->entry = gtk_entry_new();
	gtk_entry_set_placeholder_text(GTK_ENTRY(state->entry),
	 "Duration in seconds…");
	gtk_style_context_add_class(gtk_widget_get_style_context(state->entry),
	 "duration-entry");
	gtk_box_pack_start(GTK_BOX(outer), state->entry, FALSE, FALSE, 4);

	/* Button row */
	GtkWidget* btn_row =
	 gtk_box_new(GTK_ORIENTATION_HORIZONTAL, DEFAULT_SPACING);
	gtk_box_pack_start(GTK_BOX(outer), btn_row, FALSE, FALSE, 0);

	state->start_btn = gtk_button_new_with_label("▶  Start");
	gtk_style_context_add_class(gtk_widget_get_style_context(state->start_btn),
	 "start-button");
	gtk_widget_set_hexpand(state->start_btn, TRUE);
	g_signal_connect(state->start_btn, "clicked", G_CALLBACK(on_start_clicked),
	 state);
	gtk_box_pack_start(GTK_BOX(btn_row), state->start_btn, TRUE, TRUE, 0);

	state->stop_btn = gtk_button_new_with_label("⏸");
	gtk_style_context_add_class(gtk_widget_get_style_context(state->stop_btn),
	 "stop-button");
	g_signal_connect(state->stop_btn, "clicked", G_CALLBACK(on_stop_clicked),
	 state);
	gtk_box_pack_start(GTK_BOX(btn_row), state->stop_btn, FALSE, FALSE, 0);

	state->reset_btn = gtk_button_new_with_label("↺");
	gtk_style_context_add_class(gtk_widget_get_style_context(state->reset_btn),
	 "reset-button");
	g_signal_connect(state->reset_btn, "clicked", G_CALLBACK(on_reset_clicked),
	 state);
	gtk_box_pack_start(GTK_BOX(btn_row), state->reset_btn, FALSE, FALSE, 0);

	gtk_widget_show_all(state->window);
}

/* ── CLI plumbing (unchanged) ── */

static void run_timer(TimerState* state)
{
	while (state->remaining > 0) {
		if (kbhit() && getchar() == 'q') {
			printf("\nStopped by user.\n");
			return;
		}
		printf("\rRemaining: %02d:%02d:%02d",
		 state->remaining / SECONDS_PER_HOUR,
		 (state->remaining % SECONDS_PER_HOUR) / SECONDS_PER_MINUTE,
		 state->remaining % SECONDS_PER_MINUTE);
		fflush(stdout);
		sleep(1);
		state->remaining--;
	}
	printf("\nTimer ended!\n\a");
}

static void timer_cli_callback(int argc, char** argv, void* user_data)
{
	TimerState* state = (TimerState*) user_data;
	if (argc < 1 || argv[0] == NULL) {
		fprintf(stderr, "Error: No duration provided.\n");
		return;
	}
	state->remaining = (int) strtol(argv[0], NULL, BASE_TEN);
	run_timer(state);
}

static void hours_cli_callback(int argc, char** argv, void* user_data)
{
	TimerState* state = (TimerState*) user_data;
	if (argc < 1 || argv[0] == NULL) {
		fprintf(stderr, "Error: No duration provided.\n");
		return;
	}
	state->remaining = (int) strtol(argv[0], NULL, BASE_TEN) * SECONDS_PER_HOUR;
	run_timer(state);
}

static void minutes_cli_callback(int argc, char** argv, void* user_data)
{
	TimerState* state = (TimerState*) user_data;
	if (argc < 1 || argv[0] == NULL) {
		fprintf(stderr, "Error: No duration provided.\n");
		return;
	}
	state->remaining =
	 (int) strtol(argv[0], NULL, BASE_TEN) * SECONDS_PER_MINUTE;
	run_timer(state);
}

int main(int argc, char** argv)
{
	TimerState state = {
	    0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, FALSE};
	CliParser parser;
	CliInitParams params = {"timer", "A simple timer"};
	cli_init(&parser, params);

	CliArgument arg = {
	    "--set", "-s", "Set duration (s)", timer_cli_callback, &state};
	cli_add_argument(&parser, arg);

	CliArgument h_arg = {
	    "--hours", "-H", "Set duration (h)", hours_cli_callback, &state};
	cli_add_argument(&parser, h_arg);

	CliArgument m_arg = {
	    "--minutes", "-m", "Set duration (m)", minutes_cli_callback, &state};
	cli_add_argument(&parser, m_arg);

	if (argc > 1) {
		if (argv[1][0] != '-') {
			state.remaining = (int) strtol(argv[1], NULL, BASE_TEN);
			timer_cli_callback(1, &argv[1], &state);
			cli_destroy(&parser);
			return 0;
		}
		cli_parse(&parser, argc, argv);
		cli_destroy(&parser);
		return 0;
	}

	GtkApplication* app =
	 gtk_application_new("org.mini.timer", G_APPLICATION_DEFAULT_FLAGS);
	g_signal_connect(app, "activate", G_CALLBACK(activate), &state);
	int status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);
	cli_destroy(&parser);
	return status;
}
