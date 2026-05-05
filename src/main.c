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
#define DEFAULT_WINDOW_WIDTH 250
#define DEFAULT_WINDOW_HEIGHT 200
#define DEFAULT_SPACING 10
#define BORDER_WIDTH 10

typedef struct {
	int remaining;
	GtkWidget* label;
	GtkWidget* entry;
	guint timer_id;
} TimerState;

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

static gboolean handle_timer_tick(TimerState* state)
{
	if (state->remaining > 0) {
		state->remaining--;
		char buf[BUFFER_SIZE];
		snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
		 state->remaining / SECONDS_PER_HOUR,
		 (state->remaining % SECONDS_PER_HOUR) / SECONDS_PER_MINUTE,
		 state->remaining % SECONDS_PER_MINUTE);
		gtk_label_set_text(GTK_LABEL(state->label), buf);
		return TRUE;
	}
	gtk_label_set_text(GTK_LABEL(state->label), "00:00:00");
	state->timer_id = 0;
	return FALSE;
}

static void on_start_clicked(GtkWidget* widget, gpointer data)
{
	(void) widget;
	TimerState* state = (TimerState*) data;
	if (state->timer_id > 0) {
		g_source_remove(state->timer_id);
	}
	const char* text = gtk_entry_get_text(GTK_ENTRY(state->entry));
	state->remaining = (int) strtol(text, NULL, BASE_TEN);
	state->timer_id =
	 g_timeout_add_seconds(1, (GSourceFunc) handle_timer_tick, state);
}

static void activate(GtkApplication* app, gpointer user_data)
{
	TimerState* state = (TimerState*) user_data;
	GtkWidget* window = gtk_application_window_new(app);
	GtkSettings* settings = gtk_settings_get_default();
	g_object_set(settings, "gtk-application-prefer-dark-theme", TRUE, NULL);

	gtk_window_set_title(GTK_WINDOW(window), "Mini Timer");
	gtk_window_set_default_size(GTK_WINDOW(window), DEFAULT_WINDOW_WIDTH,
	 DEFAULT_WINDOW_HEIGHT);

	GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, DEFAULT_SPACING);
	gtk_container_set_border_width(GTK_CONTAINER(box), BORDER_WIDTH);
	gtk_container_add(GTK_CONTAINER(window), box);

	state->entry = gtk_entry_new();
	gtk_entry_set_placeholder_text(GTK_ENTRY(state->entry), "Seconds");
	gtk_box_pack_start(GTK_BOX(box), state->entry, FALSE, FALSE, 0);

	GtkWidget* button = gtk_button_new_with_label("Start");
	g_signal_connect(button, "clicked", G_CALLBACK(on_start_clicked), state);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);

	state->label = gtk_label_new("00:00:00");
	gtk_box_pack_start(GTK_BOX(box), state->label, TRUE, TRUE, 0);

	gtk_widget_show_all(window);
}

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
	TimerState state = {0, NULL, NULL, 0};
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
