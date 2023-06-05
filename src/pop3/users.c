#include "include/users.h"

// all users stored in system
static user_data * users;
static int users_count; 
static const char * users_file;


int initialize_users(const char * usersFile) {
    users = NULL;
    users_file = usersFile;
    users_count = 0;

    users = malloc(sizeof(user_data) * STARTING_USERS);
    if (users == NULL) {
        return -1;
    }
    load_users();

    return 0;
}

int load_users() {
    FILE * file = fopen(users_file, DEFAULT_READ);

    if (file == NULL) {
        return -1;
    }
    int to_return = 0;
    
    user_data user_data;

    while (to_return >= 0) {
        
        to_return = load_all_users(file, &user_data);
        if (to_return < 0) {
            break;
        }
        users[users_count] = user_data;
        users_count++;
    }
 
    fclose(file);
    return to_return;
}

void skip_line(FILE * file) {
    int c;
    while ((c = getc(file)) != '\n') {
        //EOF or error
        if (c < 0) {
            return;
        }
    }
}

int load_all_users(FILE * file, user_data * user_data) {
    int c;
    int username_length = 0;
    int password_length = 0;
    
    // get username
    while ((c = getc(file)) != ' ') {
        //EOF or error
        if (c < 0) {
            return -1;
        }
        user_data->username[username_length++] = c;
    }
   
    user_data->username[username_length] = '\0';
    // get password
    while ((c = getc(file)) != '\n') {
        //EOF or error
        if (c < 0) {
            return -1;
        }
        user_data->password[password_length++] = c;
    }
    user_data->password[password_length] = '\0';
    return 0;
}

int find_user(const char * username) {
    int i;
    for (i = 0; i < users_count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            return i;
        }
    }
    return -1;
}


user_status login_user(const char * username, const char * password) {
    if(users_count == 0) {
        return GENERAL_ERROR;
    }

    int user_index = find_user(username);
    if (user_index < 0)
        return USER_ERROR;
    int pass = strcmp(users[user_index].password, password);
    if(pass != 0)
        return USER_ERROR;

    return USER_OK;
}   


int finish_users() {
    free(users);
    return USER_OK;
}

/*
int main (int argc, char * argv[]) {
    initialize_users("names.txt");
    int login_status = login_user("Garcos", "Mronda");
    printf("Username: Garcos\nPassword: Mronda\nLogin status: %d\n", login_status);
    if(!login_status) {
        printf("Proceed to access user mail directory and secure lock of it\n");
        printf("Then, return to command line to receive pop3 and statistics commands\n");
    }
    printf("All users in heap memory\n");
    for( int i = 0 ; i < users_count ; i++ ) {
        printf("username: %s\n", users[i].username);
        printf("password: %s\n", users[i].password);
    }

    finish_users();
}
*/