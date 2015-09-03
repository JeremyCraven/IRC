
#include <stdio.h>
#include <gtk/gtk.h>
#include "Client.h"
#include "IRCClient.h"

char * host;
char * master_user;
char * master_password;
char * current_room = strdup("");
int port;
int quit = 0;

GtkListStore * list_rooms;
GtkListStore * list_users;
GtkWidget * window;
GtkWidget * send_view;
GtkWidget * message_view;
GtkWidget * c_user;
GtkWidget * c_room;


/* Callback for invalid user input */
void invalid_entry() {
    GtkWidget * dialog;
    GtkWidget * message_label;
    GtkWidget * hbox1;

    dialog = gtk_dialog_new_with_buttons("Invalid Login Attempt", GTK_WINDOW(window), GTK_DIALOG_MODAL, "CONTINUE", GTK_RESPONSE_YES,
					 "QUIT", GTK_RESPONSE_NO, NULL);
    message_label = gtk_label_new("INVALID LOGIN ATTEMPT\nDo you want to continue?");
    hbox1 = gtk_hbox_new(TRUE, 2);
    
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox1, TRUE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox1), message_label, TRUE, FALSE, 5);
    gtk_widget_show_all(dialog);

    gint result = gtk_dialog_run(GTK_DIALOG(dialog));

    switch (result) {
	case GTK_RESPONSE_YES: {
	    gtk_widget_destroy(dialog);
	    login_event();
	    break;
	}
	case GTK_RESPONSE_NO: {
	    gtk_widget_destroy(dialog);
	    quit = 1;
	    break;
	}
	case GTK_RESPONSE_DELETE_EVENT: {
	    quit = 1;
	}
    }
}

/* Updates the client's room list */
void update_list_rooms() {
    GtkTreeIter iter;
    
    char * command = (char *)malloc(MAX_COMMAND * sizeof(char));
    char * response = (char *)malloc(MAX_RESPONSE * sizeof(char));

    sprintf(command, "%s %s %s", "LIST-ROOMS", master_user, master_password);
    gtk_list_store_clear(list_rooms);    
    sendCommand((char *)host, port, (char *)command, response);

    char * token = strtok(response, " \r\n");
    token = strtok(NULL, " \r\n");
    token = strtok(NULL, " \r\n");
    token = strtok(NULL, "\r\n");

    while (token != NULL) {
	gchar * msg = g_strdup_printf("%s", token);
	gtk_list_store_append(GTK_LIST_STORE(list_rooms), &iter);
	gtk_list_store_set(GTK_LIST_STORE(list_rooms), &iter, 0, msg, -1);
	token = strtok(NULL, "\r\n");
	g_free(msg);
    }

    g_free(response);
    g_free(command);
}

/* Updates the client's user list */
void update_list_users() {
    GtkTreeIter iter;

    char * command = (char *)malloc(MAX_COMMAND * sizeof(char));
    char * response = (char *)malloc(MAX_RESPONSE * sizeof(char));

    sprintf(command, "%s %s %s %s", "GET-USERS-IN-ROOM", master_user, master_password, current_room);

    gtk_list_store_clear(list_users);    
    sendCommand(host, port, command, response);

    if (strcmp(response, "NO ROOM ENTERED\r\n") == 0) {
	free(command);
	free(response);
	return;
    }

    char * token = strtok(response, " \r\n");

    while (token != NULL) {
	gchar * msg = g_strdup_printf("%s", token);
	gtk_list_store_append(GTK_LIST_STORE(list_users), &iter);
	gtk_list_store_set(GTK_LIST_STORE(list_users), &iter, 0, msg, -1);
	token = strtok(NULL, "\r\n");
	g_free(msg);
    }

    g_free(response);
    g_free(command);
}

/* Updates the list of messages */
void update_list_messages() {
    GtkTextBuffer * buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(message_view));
    char * command = (char *)malloc(MAX_COMMAND * sizeof(char));
    char * response = (char *)malloc(MAX_RESPONSE * sizeof(char));

    if (strcmp(current_room, "") == 0) {
	return;	
    }

    GtkTextIter start;
    GtkTextIter end;
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_delete(buffer, &start, &end);

    sprintf(command, "%s %s %s %s %s", "GET-MESSAGES", master_user, master_password, "0", current_room);
    sendCommand(host, port, command, response);

    if (strcmp(response, "ERROR (User not in room)\r\n") == 0) {
	free(command);
	free(response);
	return;
    }

    char * token = strtok(response, " ");
    token = strtok(NULL, " ");
    char * name = token;
    token = strtok(NULL, "\r\n");
    char * message = token;

    while (token != NULL) {
	gchar * msg = g_strdup_printf("%s: %s\r\n", name, message);
	insert_text(buffer, msg);
	token = strtok(NULL, " ");
	token = strtok(NULL, " ");
	name = token;
	token = strtok(NULL, "\r\n");
	message = token;
	g_free(msg);
    }
}

void update_current() {
    char * r = (char *)malloc(MAX_COMMAND * sizeof(char));
    char * u = (char *)malloc(MAX_COMMAND * sizeof(char));

    sprintf(r, "%s %s", "CURRENT USER:", master_user);
    sprintf(u, "%s %s", "CURRENT ROOM:", current_room);
    
    gtk_label_set_text(GTK_LABEL(c_user), r);
    gtk_label_set_text(GTK_LABEL(c_room), u);

    free(r);
    free(u);
}

/* Create a new user */
static void create_user_event(GtkWidget * widget, gpointer data) {
    GtkWidget * dialog;
    GtkWidget * username_label;
    GtkWidget * password_label;
    GtkWidget * username_entry;
    GtkWidget * password_entry;
    GtkWidget * hbox1;
    GtkWidget * hbox2;

    dialog = gtk_dialog_new_with_buttons("Create User", GTK_WINDOW(window), GTK_DIALOG_MODAL, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
    username_label = gtk_label_new("Username:");
    password_label = gtk_label_new("Password:");
    username_entry = gtk_entry_new();
    password_entry = gtk_entry_new();
    hbox1 = gtk_hbox_new(TRUE, 2);
    hbox2 = gtk_hbox_new(TRUE, 2);
    
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox1, TRUE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox1), username_label, TRUE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox1), username_entry, TRUE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox2, TRUE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox2), password_label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox2), password_entry, FALSE, FALSE, 5);    
    gtk_widget_show_all(dialog);

    gint result = gtk_dialog_run(GTK_DIALOG(dialog));

    switch (result) {
	case GTK_RESPONSE_OK: {
	    const gchar * username = gtk_entry_get_text(GTK_ENTRY(username_entry));
	    const gchar * pass = gtk_entry_get_text(GTK_ENTRY(password_entry)); 
	    gchar * response = (char *)malloc(MAX_RESPONSE * sizeof(char));
	    gchar * command = (char *)malloc(MAX_COMMAND * sizeof(char));

	    sprintf(command, "%s %s %s", "ADD-USER", username, pass);
	    sendCommand(host, port, command, response);

	    master_user = strdup(username);
	    master_password = strdup(pass);

	    gtk_widget_destroy(dialog);
	    free(response);
	    free(command);
	    break;
	}
	case GTK_RESPONSE_DELETE_EVENT: {
	    gtk_widget_destroy(dialog);
	}
    }
}

/* Create a new room */
static void create_room_event(GtkWidget * widget, gpointer data) {
    GtkWidget * dialog;
    GtkWidget * room_label;
    GtkWidget * room_entry;
    GtkWidget * hbox1;

    dialog = gtk_dialog_new_with_buttons("Create Room", GTK_WINDOW(window), GTK_DIALOG_MODAL, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
    room_label = gtk_label_new("Room:");
    room_entry = gtk_entry_new();
    hbox1 = gtk_hbox_new(TRUE, 2);
    
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox1, TRUE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox1), room_label, TRUE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox1), room_entry, TRUE, FALSE, 5);
    gtk_widget_show_all(dialog);

    gint result = gtk_dialog_run(GTK_DIALOG(dialog));

    switch (result) {
	case GTK_RESPONSE_OK: {
	    if (gtk_entry_get_text_length(GTK_ENTRY(room_entry)) == 0) {
	 	gtk_widget_destroy(dialog);
		break;
	    }

	    const gchar * room = gtk_entry_get_text(GTK_ENTRY(room_entry)); 
	    gchar * response = (char *)malloc(MAX_RESPONSE * sizeof(char));
	    gchar * command = (char *)malloc(MAX_COMMAND * sizeof(char));

	    sprintf(command, "%s %s %s %s", "CREATE-ROOM", master_user, master_password, room);
	    sendCommand(host, port, command, response);

	    gtk_widget_destroy(dialog);
	    free(response);
	    free(command);
	    break;
	}
	case GTK_RESPONSE_DELETE_EVENT: {
	    gtk_widget_destroy(dialog);
	}
    }
}

static void send_message_event(GtkWidget * widget, gpointer data) {
    GtkTextIter begin;
    GtkTextIter end;

    if (strcmp(current_room, "") == 0) {
	GtkWidget * dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
						    "Enter a room before sending a message.", NULL); 
	gint result = gtk_dialog_run(GTK_DIALOG(dialog));

	switch (result) {
	    case GTK_RESPONSE_OK: {
		gtk_widget_destroy(dialog);
		break;
	    }
	    case GTK_RESPONSE_DELETE_EVENT: 
		gtk_widget_destroy(dialog);
	}

	return;
    }

    gchar * response = (char *)malloc(MAX_RESPONSE * sizeof(char));
    gchar * command = (char *)malloc(MAX_COMMAND * sizeof(char));
    GtkTextBuffer * buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(send_view));

    gtk_text_buffer_get_start_iter(buffer, &begin);
    gtk_text_buffer_get_end_iter(buffer, &end);
    char * message = gtk_text_buffer_get_text(buffer, &begin, &end, FALSE);

    sprintf(command, "%s %s %s %s %s", "SEND-MESSAGE", master_user, master_password, current_room, message);
    sendCommand(host, port, command, response);
    free(message);
    free(command);
    free(response);
}

void on_changed_rooms(GtkWidget * widget, gpointer data) {
    GtkTreeIter iter;
    GtkTreeModel * model;
    char * room;

    if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(widget), &model, &iter)) {
	gtk_tree_model_get(model, &iter, 0, &room, -1);
	gchar * response = (char *)malloc(MAX_RESPONSE * sizeof(char));
	gchar * command = (char *)malloc(MAX_COMMAND * sizeof(char));
	gchar * msg = (char *)malloc(MAX_COMMAND * sizeof(char));

	if (strcmp(current_room, "")) {
	    sprintf(msg, "%s", "USER HAS LEFT THE ROOM.");
	    sprintf(command, "%s %s %s %s %s", "SEND-MESSAGE", master_user, master_password, current_room, msg);
	    sendCommand(host, port, command, response);
	}

	sprintf(command, "%s %s %s %s", "LEAVE-ROOM", master_user, master_password, current_room);
	sendCommand(host, port, command, response);

	sprintf(command, "%s %s %s %s", "ENTER-ROOM", master_user, master_password, room);
	sendCommand(host, port, command, response);

	sprintf(msg, "%s", "USER HAS ENTERED THE ROOM.");
	sprintf(command, "%s %s %s %s %s", "SEND-MESSAGE", master_user, master_password, room, msg);
	sendCommand(host, port, command, response);

	current_room = strdup(room);

	free(command);
	free(response);
	free(msg);
	free(room);	
    }
} 

/* Create the list of messages */
static GtkWidget * create_list(const char * titleColumn, GtkListStore * model) {
    GtkWidget * scrolled_window;
    GtkWidget * tree_view;
    GtkCellRenderer * cell;
    GtkTreeViewColumn * column;
    GtkTreeSelection * selection;
   
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   
    tree_view = gtk_tree_view_new();
    gtk_container_add(GTK_CONTAINER(scrolled_window), tree_view);
    gtk_tree_view_set_model(GTK_TREE_VIEW(tree_view), GTK_TREE_MODEL(model));
    gtk_widget_show(tree_view);
   
    cell = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(titleColumn, cell, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), GTK_TREE_VIEW_COLUMN(column));

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));

    if (strcmp(titleColumn, "Rooms") == 0) {
	g_signal_connect(selection, "changed", G_CALLBACK(on_changed_rooms), NULL);
    }

    return scrolled_window;
}

/* Add text to text widget */
static void insert_text(GtkTextBuffer * buffer, const char * initialText) {
    GtkTextIter iter;
 
    gtk_text_buffer_get_iter_at_offset(buffer, &iter, 0);
    gtk_text_buffer_insert(buffer, &iter, initialText, -1);
}
   
/* Create a scrolled text area that displays a message */
static GtkWidget * create_text(const char * initialText) {
    GtkWidget * scrolled_window;
    GtkTextBuffer * buffer;

    message_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(message_view), FALSE);   
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(message_view));
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled_window), message_view);
    insert_text(buffer, initialText);
    gtk_widget_show_all(scrolled_window);

    return scrolled_window;
}

/* Create a scrolled text area that displays the message to be sent */
static GtkWidget * create_text_send(const char * initialText) {
    GtkWidget * scrolled_window;
    GtkTextBuffer * buffer;

    send_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(send_view), TRUE);   
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(send_view));
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled_window), send_view);
    insert_text(buffer, initialText);
    gtk_widget_show_all(scrolled_window);

    return scrolled_window;
}

/* Updates client at 5 second intervals */
static gboolean time_handler(GtkWidget * widget) {
    if (widget->window == NULL)
	return FALSE;

    update_list_rooms();
    update_list_users();
    update_list_messages();
    update_current();
    
    return TRUE;
}

/* Login screen upon starting client */
static void login_event() {
    GtkWidget * dialog;
    GtkWidget * username_label;
    GtkWidget * password_label;
    GtkWidget * host_label;
    GtkWidget * port_label;
    GtkWidget * username_entry;
    GtkWidget * password_entry;
    GtkWidget * host_entry;
    GtkWidget * port_entry;
    GtkWidget * hbox1;
    GtkWidget * hbox2;
    GtkWidget * hbox3;
    GtkWidget * hbox4;

    dialog = gtk_dialog_new_with_buttons("User Login", GTK_WINDOW(window), GTK_DIALOG_MODAL, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
    username_label = gtk_label_new("Username:");
    password_label = gtk_label_new("Password:");
    host_label = gtk_label_new("Host:");
    port_label = gtk_label_new("Port:");
    username_entry = gtk_entry_new();
    password_entry = gtk_entry_new();
    host_entry = gtk_entry_new();
    port_entry = gtk_entry_new();
    hbox1 = gtk_hbox_new(TRUE, 2);
    hbox2 = gtk_hbox_new(TRUE, 2);
    hbox3 = gtk_hbox_new(TRUE, 2);
    hbox4 = gtk_hbox_new(TRUE, 2);
    
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox1, TRUE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox1), username_label, TRUE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox1), username_entry, TRUE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox2, TRUE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox2), password_label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox2), password_entry, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox3, TRUE, FALSE, 5);    
    gtk_box_pack_start(GTK_BOX(hbox3), host_label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox3), host_entry, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox4, TRUE, FALSE, 5);    
    gtk_box_pack_start(GTK_BOX(hbox4), port_label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox4), port_entry, FALSE, FALSE, 5);

    gtk_widget_show_all(dialog);

    gint result = gtk_dialog_run(GTK_DIALOG(dialog));

    switch (result) {
	case GTK_RESPONSE_OK: {
	    if (gtk_entry_get_text_length(GTK_ENTRY(username_entry)) == 0) {
	 	gtk_widget_destroy(dialog);
		invalid_entry();
		break;
	    }
	    if (gtk_entry_get_text_length(GTK_ENTRY(password_entry)) == 0) {
		gtk_widget_destroy(dialog);
	 	invalid_entry();
		break;
	    }
	    if (gtk_entry_get_text_length(GTK_ENTRY(host_entry)) == 0) {
		gtk_widget_destroy(dialog);
	 	invalid_entry();
		break;
	    }
	    if (gtk_entry_get_text_length(GTK_ENTRY(port_entry)) == 0) {
		gtk_widget_destroy(dialog);
	 	invalid_entry();
		break;
	    }
	    
	    master_user = strdup(gtk_entry_get_text(GTK_ENTRY(username_entry)));
	    master_password = strdup(gtk_entry_get_text(GTK_ENTRY(password_entry)));
	    host = strdup(gtk_entry_get_text(GTK_ENTRY(host_entry)));
	    port = atoi(strdup(gtk_entry_get_text(GTK_ENTRY(port_entry)))); 
	    gchar * response = (char *)malloc(MAX_RESPONSE * sizeof(char));
	    gchar * command = (char *)malloc(MAX_COMMAND * sizeof(char));
	    
	    sprintf(command, "%s %s %s", "ADD-USER", master_user, master_password);
	    int valid = sendCommand(host, port, command, response);

	    if (valid == 0) {
		gtk_widget_destroy(dialog);
		invalid_entry();
		break;		
	    }

	    sprintf(command, "%s %s %s", "CHECK-PASSWORD", master_user, master_password);
	    sendCommand(host, port, command, response);

	    if (strcmp(response, "ERROR (Wrong password)\r\n") == 0) {
		gtk_widget_destroy(dialog);
		invalid_entry();
		break;
	    }

	    gtk_widget_destroy(dialog);
	    break;
	}
	case GTK_RESPONSE_DELETE_EVENT: {
	    gtk_widget_destroy(dialog);
	    quit = 1;
	    break;
	}
	default: {
	    gtk_widget_destroy(dialog);
	}
    }
}

int main(int argc, char * argv[]) {
    GtkWidget * roomList;
    GtkWidget * userList;
    GtkWidget * messages;
    GtkWidget * myMessage;
    GtkWidget * messages_label;
    GtkWidget * send_messages_label;

    gtk_init(&argc, &argv);

    login_event();

    if (quit == 1) {
	return 0;
    }
   
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "IRC Client");
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    gtk_widget_set_size_request(GTK_WIDGET(window), 600, 650);

    // Create a table to place the widgets. Use a 10x4 Grid (10 rows x 4 columns)
    GtkWidget * table = gtk_table_new(10, 4, TRUE);
    gtk_container_add(GTK_CONTAINER(window), table);
    gtk_table_set_row_spacings(GTK_TABLE(table), 5);
    gtk_table_set_col_spacings(GTK_TABLE(table), 5);
    gtk_widget_show(table);

    // Add labels
    messages_label = gtk_label_new("MESSAGES:");
    send_messages_label = gtk_label_new("TYPE A MESSAGE:");
    c_user = gtk_label_new(NULL);
    c_room = gtk_label_new(NULL);
    gtk_table_attach_defaults(GTK_TABLE(table), messages_label, 0, 1, 2, 3);
    gtk_table_attach_defaults(GTK_TABLE(table), send_messages_label, 0, 1, 6, 7);
    gtk_widget_show(send_messages_label);
    gtk_widget_show(messages_label);
    gtk_table_attach_defaults(GTK_TABLE(table), c_user, 0, 2, 10, 11);
    gtk_table_attach_defaults(GTK_TABLE(table), c_room, 2, 4, 10, 11);
    gtk_widget_show(c_user);
    gtk_widget_show(c_room);
    update_current();

    // Add list of rooms. Use columns 2 to 4 (exclusive) and rows 0 to 2 (exclusive)
    list_rooms = gtk_list_store_new(1, G_TYPE_STRING);
    update_list_rooms();
    roomList = create_list("Rooms", list_rooms);
    gtk_table_attach_defaults(GTK_TABLE(table), roomList, 2, 4, 0, 2);
    gtk_widget_show(roomList);

    // Add list of users.
    list_users = gtk_list_store_new(1, G_TYPE_STRING);
    update_list_users();
    userList = create_list("Users", list_users);
    gtk_table_attach_defaults(GTK_TABLE(table), userList, 0, 2, 0, 2);    
    gtk_widget_show(userList);
   
    // Add messages text. Use columns 0 to 4 (exclusive) and rows 4 to 7 (exclusive) 
    messages = create_text("");
    gtk_table_attach_defaults(GTK_TABLE(table), messages, 0, 4, 3, 6);
    update_list_messages();
    gtk_widget_show(messages);

    // Add send message text. Use columns 0 to 4 (exclusive) and rows 4 to 7 (exclusive)
    myMessage = create_text_send("");
    gtk_table_attach_defaults(GTK_TABLE(table), myMessage, 0, 4, 7, 9);
    gtk_widget_show(myMessage);

    // Add send button. Use columns 0 to 1 (exclusive) and rows 7 to 8 (exclusive)
    GtkWidget * send_button = gtk_button_new_with_label("Send");
    gtk_table_attach_defaults(GTK_TABLE(table), send_button, 0, 1, 9, 10); 
    gtk_widget_show(send_button);
    g_signal_connect(send_button, "clicked", G_CALLBACK(send_message_event), NULL);

    // Add create user button. Use columns 3 to 4 (exclusive) and rows 7 to 8 (exclusive)
    GtkWidget * create_user_button = gtk_button_new_with_label("Create Account");
    gtk_table_attach_defaults(GTK_TABLE(table), create_user_button, 3, 4, 9, 10);
    gtk_widget_show(create_user_button);
    g_signal_connect(create_user_button, "clicked", G_CALLBACK(create_user_event), NULL);

    // Add create room button. Use columns 2 to 3 (exclusive) and rows 7 to 8 (exclusive)
    GtkWidget * create_room_button = gtk_button_new_with_label("Create Room");
    gtk_table_attach_defaults(GTK_TABLE(table), create_room_button, 2, 3, 9, 10);
    gtk_widget_show(create_room_button);
    g_signal_connect(create_room_button, "clicked", G_CALLBACK(create_room_event), NULL);

    gtk_widget_show(table);
    gtk_widget_show(window);

    g_timeout_add(5000, (GSourceFunc)time_handler, (gpointer)window);

    gtk_main();

    return 0;
}

