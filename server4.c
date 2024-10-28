#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include<signal.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define MAX_SEATS 20
#define MAX_WAITING_LIST 20
#define MAX_USERS 20
#define MAX_COACHES 3
#define MAX_ROWS 4
#define MAX_COLS 5

// User structure
typedef struct {
    char username[100];
    char password[100];
} User;

// Passenger structure
typedef struct {
    char username[100];
    char name[100];
    int age;
    int pnr;
    int seat_num;
    int coach_num;
    int is_waitlisted;
    float price;
} Passenger;

User users[MAX_USERS];
int user_count = 0;
int logged_in_user=-1;
char logged_username[100];
Passenger seats[MAX_COACHES][MAX_SEATS];
Passenger waiting_list[MAX_WAITING_LIST];
Passenger priority_queue[MAX_WAITING_LIST];

int waiting_list_count = 0;
int priority_queue_count = 0;
int pnr_counter = 1000;
int available_seats[MAX_COACHES];

// File paths for text-based storage
char *user_file = "users.txt";
char *reservation_file = "reservations.txt";
char *prices_file = "prices.txt";

int hash_password(char *password) {
    int hash = 0;
    while (*password) {
        hash += *password++;
    }
    return hash;
}

// Function to load users from text file
void load_users() {
    FILE *file = fopen(user_file, "r");
    if (file == NULL) return;

    fscanf(file, "%d", &user_count);
    for (int i = 0; i < user_count; i++) {
        fscanf(file, "%s %s", users[i].username, users[i].password);
    }
    fclose(file);
}

// Function to save users to text file
void save_users() {
    FILE *file = fopen(user_file, "w");
    fprintf(file, "%d\n", user_count);
    for (int i = 0; i < user_count; i++) {
        fprintf(file, "%s %s\n", users[i].username, users[i].password);
    }
    fclose(file);
}

// Function to load reservations from text file
void load_reservations() {
    FILE *file = fopen(reservation_file, "r");
    if (file == NULL) return;

    fscanf(file, "%d %d %d", &waiting_list_count, &priority_queue_count, &pnr_counter);
    for (int i = 0; i < MAX_COACHES; i++) {
        fscanf(file, "%d", &available_seats[i]);
        printf("LOADing coach %d with %d available seats\n", i, available_seats[i]);
        for (int j = 0; j < MAX_SEATS; j++) {
            fscanf(file, "%d %s %s %d %d %d %d %f",
                &seats[i][j].pnr, seats[i][j].username, seats[i][j].name, &seats[i][j].age,
                &seats[i][j].seat_num, &seats[i][j].coach_num, &seats[i][j].is_waitlisted, &seats[i][j].price);
        }
        }
    

    for (int i = 0; i < waiting_list_count; i++) {
        fscanf(file, "%d %s %s %d %d %d %d %f",
            &waiting_list[i].pnr, waiting_list[i].username, waiting_list[i].name, &waiting_list[i].age,
            &waiting_list[i].seat_num, &waiting_list[i].coach_num, &waiting_list[i].is_waitlisted,&waiting_list[i].price);
    }

    for (int i = 0; i < priority_queue_count; i++) {
        fscanf(file, "%d %s %s %d %d %d %d %f",
            &priority_queue[i].pnr, priority_queue[i].username, priority_queue[i].name, &priority_queue[i].age,
            &priority_queue[i].seat_num, &priority_queue[i].coach_num, &priority_queue[i].is_waitlisted, &priority_queue[i].price);
    }

    fclose(file);
}

// Function to save reservations to text file
void save_reservations() {
    FILE *file = fopen(reservation_file, "w");
    fprintf(file, "%d %d %d\n", waiting_list_count, priority_queue_count, pnr_counter);
    
    for (int i = 0; i < MAX_COACHES; i++) {
        fprintf(file, "%d\n", available_seats[i]);
       // printf("SAVing coach %d with %d available seats\n", i, available_seats[i]);
        for (int j = 0; j < MAX_SEATS; j++) {
            fprintf(file, "%d %s %s %d %d %d %d %f\n",
                seats[i][j].pnr, seats[i][j].username, seats[i][j].name, seats[i][j].age,
                seats[i][j].seat_num, seats[i][j].coach_num, seats[i][j].is_waitlisted, seats[i][j].price);
        }
        
    }

    for (int i = 0; i < waiting_list_count; i++) {
        fprintf(file, "%d %s %s %d %d %d %d %f\n",
            waiting_list[i].pnr, waiting_list[i].username, waiting_list[i].name, waiting_list[i].age,
            waiting_list[i].seat_num, waiting_list[i].coach_num, waiting_list[i].is_waitlisted, waiting_list[i].price);
    }

    for (int i = 0; i < priority_queue_count; i++) {
        fprintf(file, "%d %s %s %d %d %d %d %f\n",
            priority_queue[i].pnr, priority_queue[i].username, priority_queue[i].name, priority_queue[i].age,
            priority_queue[i].seat_num, priority_queue[i].coach_num, priority_queue[i].is_waitlisted,priority_queue[i].price);
    }

    fclose(file);
}

void register_user(int sock, const char *username,char *password) {
    if (user_count >= MAX_USERS) {
        printf("User registration full. Cannot add more users.\n");
        return;
    }
    User new_user;
    strcpy(new_user.username,username);
    sprintf(new_user.password, "%d", hash_password(password));
    users[user_count++] = new_user;
    save_users(); 
    send(sock, "Registration successful.\n", 25, 0);
}

// Function to log in a user
int login_user(int sock, const char *username, char *password) {
    char hashed_password[100];
    sprintf(hashed_password, "%d", hash_password(password));

    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0 && strcmp(users[i].password, hashed_password) == 0) {
            logged_in_user = i;
            strcpy(logged_username,username);
            return 1;
        }
    }

    return 0;
}

void display_seat_matrix(int sock,int coach_num) {
    if (coach_num < 0 || coach_num >= MAX_COACHES) {
        send(sock,"Invalid coach number.\n",50,0);
        return;
    }

    char buffer[1024]={0};
    snprintf(buffer, sizeof(buffer), "\n-- Seat Availability Matrix --\n");
    send(sock, buffer, strlen(buffer), 0);
        snprintf(buffer, sizeof(buffer), " Coach %d:\n", coach_num + 1);
        send(sock, buffer, strlen(buffer), 0);
        for(int j=0;j<MAX_ROWS;j++){
        for (int i = 0; i < MAX_COLS; i++) {
                if (seats[coach_num][i + j * MAX_COLS].pnr != 0) {
                snprintf(buffer, sizeof(buffer), "[X]\t");
                send(sock, buffer, strlen(buffer), 0);
            } else {
                snprintf(buffer, sizeof(buffer), "[ ]\t");
                send(sock, buffer, strlen(buffer), 0);
            }
        }
        send(sock, "\n", 1, 0);
        }
    
}


int generate_pnr() {
    return pnr_counter++;
}

float get_price(int coach_num){
FILE *file = fopen(prices_file, "r");
    if (!file) {
        printf("Failed to open pricing file.\n");
        return 0.0;
    }

    float price = 0.0, c_price=0.0;
    int c_num;
     for (int i = 0; i < MAX_COACHES; i++) {
        fscanf(file, "%d %f",&c_num,&c_price); 
        if (c_num==coach_num) {
         price=c_price;
        }
     }
      if (available_seats[coach_num] > 0) {
        float increment = 0.1+(MAX_SEATS-available_seats[coach_num]);
        price=price*increment;
      }


    fclose(file);
    return price;
}


void reserve_seat(int sock, const char*username,const char* name, const int age, const int coach_num) {
    if (logged_in_user == -1) {
        send(sock,"Please log in before reserving a seat.\n",50,0);
        return;
    }
    char buffer[1024]={0};
    Passenger p;
    strcpy(p.name,name);
    p.age=age;

    if (coach_num < 0 || coach_num >= MAX_COACHES) {
        send(sock,"Invalid coach number.\n",50,0);
        return;
    }

    p.pnr = generate_pnr();
    p.coach_num = coach_num;
    p.is_waitlisted = 0;
    strcpy(p.username, users[logged_in_user].username);
    p.price=get_price(coach_num);

    if (available_seats[coach_num] > 0) {
        p.seat_num = MAX_SEATS - available_seats[coach_num] + 1;
        seats[coach_num][MAX_SEATS - available_seats[coach_num]] = p;
        available_seats[coach_num]--;
        snprintf(buffer,sizeof(buffer),"Seat allocated to %s (PNR: %d, Seat No: %d, Coach: %d)\nPrice:%.2f", p.name, p.pnr, p.seat_num, coach_num + 1,p.price);
        send(sock, buffer, strlen(buffer), 0);
    } else {
        snprintf(buffer,sizeof(buffer),"No seats available. Adding to waiting list.\nPNR: %d\n", p.pnr);
        send(sock, buffer, strlen(buffer), 0);
        p.is_waitlisted = 1;
        p.seat_num=-1;
        if(p.age>=60){
         priority_queue[priority_queue_count++]=p;
        }
        else{
        waiting_list[waiting_list_count++] = p;
        }
    }

    save_reservations();  
}

void check_reservation(int sock, const char* username, const int pnr) {
    
    if (logged_in_user == -1) {
         send(sock,"Please log in before reserving a seat.\n",50,0);
        return;
    }

    char buffer[1024]={0};
    for (int coach_num = 0; coach_num < MAX_COACHES; coach_num++) {
        for (int i = 0; i < MAX_SEATS; i++) {
            if (seats[coach_num][i].pnr == pnr && strcmp(seats[coach_num][i].username, logged_username) == 0) {
               // float price = get_price(coach_num);
                snprintf(buffer,sizeof(buffer),"Reservation found for username: %s! \nName: %s, Age: %d, Seat No: %d, Coach: %d\nPrice: Rs %.2f",seats[coach_num][i].username,
                    seats[coach_num][i].name, seats[coach_num][i].age, seats[coach_num][i].seat_num, coach_num + 1,seats[coach_num][i].price);
                send(sock,buffer, strlen(buffer), 0);
                return;
            }
        }
    }

    for (int i = 0; i < waiting_list_count; i++) {
        if (waiting_list[i].pnr == pnr) {
            snprintf(buffer,sizeof(buffer),"You are on the waiting list. \nName: %s, Age: %d\n", waiting_list[i].name, waiting_list[i].age);
              send(sock,buffer, strlen(buffer), 0);
            return;
        }
    }

     snprintf(buffer,sizeof(buffer),"No reservation found for PNR: %d\n", pnr);
     send(sock,buffer, strlen(buffer), 0);
}


void cancel_reservation(int sock, const char* username, const int pnr) {
    int found = 0;
    char buffer[1024] = {0};

    // Search for the reservation in the seats array
    for (int coach_num = 0; coach_num < MAX_COACHES; coach_num++) {
        for (int i = 0; i < MAX_SEATS; i++) {
            if (seats[coach_num][i].pnr == pnr && strcmp(seats[coach_num][i].username, username) == 0) {
                // Reservation found; cancel it
                snprintf(buffer, sizeof(buffer), "Reservation for %s (PNR: %d, Seat: %d, Coach: %d) cancelled.\n", seats[coach_num][i].name, pnr, seats[coach_num][i].seat_num, coach_num + 1);
                send(sock, buffer, strlen(buffer), 0);
                
                // Free up the seat
                seats[coach_num][i].pnr = 0;
                available_seats[coach_num]++;

                // Check priority queue for possible reallocation
                if (priority_queue_count > 0 && priority_queue[0].coach_num == coach_num) {
                    // Reallocate seat to priority queue passenger
                    priority_queue[0].seat_num = seats[coach_num][i].seat_num;
                    priority_queue[0].is_waitlisted = 0;
                    seats[coach_num][i] = priority_queue[0];
                    available_seats[coach_num]--;

                    // Shift priority queue
                    for (int j = 1; j < priority_queue_count; j++) {
                        priority_queue[j - 1] = priority_queue[j];
                    }
                    priority_queue_count--;
                }
                // If no one in priority queue, check waiting list
                else if (waiting_list_count > 0 && waiting_list[0].coach_num == coach_num) {
                    // Reallocate seat to waiting list passenger
                    waiting_list[0].seat_num = seats[coach_num][i].seat_num;
                    waiting_list[0].is_waitlisted = 0;
                    seats[coach_num][i] = waiting_list[0];
                    available_seats[coach_num]--;

                    // Shift waiting list
                    for (int j = 1; j < waiting_list_count; j++) {
                        waiting_list[j - 1] = waiting_list[j];
                    }
                    waiting_list_count--;

                }

                found = 1;
                save_reservations();  // Save updated reservation data
                break;
            }
        }
        if (found) break;
    }

    // If reservation was not found in the seats array, check waiting list
    if (!found) {
        for (int i = 0; i < waiting_list_count; i++) {
            if (waiting_list[i].pnr == pnr && strcmp(waiting_list[i].username, username) == 0) {
                snprintf(buffer, sizeof(buffer), "Reservation for %s (PNR: %d) cancelled from waiting list.\n", waiting_list[i].name, pnr);
                send(sock, buffer, strlen(buffer), 0);

                // Remove passenger from waiting list
                for (int j = i + 1; j < waiting_list_count; j++) {
                    waiting_list[j - 1] = waiting_list[j];
                }
                waiting_list_count--;

                found = 1;
                save_reservations();  // Save updated reservation data
                break;
            }
        }
    }

    // If no reservation was found
    if (!found) {
        snprintf(buffer, sizeof(buffer), "PNR not found for this username.\n");
        send(sock, buffer, strlen(buffer), 0);
    }
}

// Function to handle client requests
void handle_client(int sock) {
     char buffer[1024]={0};
    int valread;

    // Read the client's request
    while ((valread = read(sock, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[valread] = '\0';
        printf("Request from client: %s\n", buffer);

        // Command parsing
        char command[20], username[50], password[50];
        sscanf(buffer, "%s", command);

        if (strcmp(command, "REGISTER") == 0) {
            sscanf(buffer + strlen(command) + 1, "%s %s", username, password);
            register_user(sock, username, password);
        } 
        else if (strcmp(command, "LOGIN") == 0) {
            sscanf(buffer + strlen(command) + 1, "%s %s", username, password);
            if (login_user(sock, username, password)) {
                send(sock, "Login successful.\n", 18, 0);
            } else {
                send(sock, "Login failed.\n", 15, 0);
            }
        }
        
        else if (strcmp(command, "DISPLAY_SEATS") == 0) {
            int coach_num;
            sscanf(buffer, "DISPLAY_SEATS %d",&coach_num);
            display_seat_matrix(sock,coach_num);
        }
        
        else if (strcmp(command, "BOOK") == 0) {
            char name[50];
            int age,coach_num;
            sscanf(buffer, "BOOK %s %s %d %d",username,name,&age,&coach_num);
            reserve_seat(sock, username, name, age,coach_num);
        }
        
        else if (strcmp(command, "CANCEL") == 0) {
            int pnr;
            sscanf(buffer, "CANCEL %s %d",username,&pnr);
            cancel_reservation(sock, username,pnr);
        }
        
        else if (strcmp(command, "RES_STATUS") == 0) {
            int pnr;
            sscanf(buffer, "RES_STATUS %s %d",username,&pnr);
            check_reservation(sock, username,pnr);
        }
        else if (strcmp(command, "EXIT") == 0) {
            save_reservations();
        }
        
        
        else {
            send(sock, "Unknown command\n", 16, 0);
        }
        memset(buffer, 0, sizeof(buffer));
    }

    close(sock);
}


int server_fd;

// Signal handler to catch Ctrl + C (SIGINT)
void handle_sigint(int sig) {
    printf("\nCaught signal %d. Closing the server socket...\n", sig);

    // Close the server socket
    if (server_fd >= 0) {
        save_reservations();
        close(server_fd);
        printf("Server socket closed.\n");
    }
    save_reservations();
    close(server_fd);

    exit(0); 
}

int main() {
 //initialize_coaches();
 signal(SIGINT, handle_sigint);
 signal(SIGTERM, handle_sigint);

    // Server code here
    int client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    load_users();
    load_reservations();
    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to the port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Start listening for incoming connections
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    // Accept incoming connections and create new thread to handle each client
    while ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) >= 0) {
        pthread_t client_thread;
        printf("New client connected.\n");
        pthread_create(&client_thread, NULL, (void *)handle_client, (void *)(intptr_t)client_socket);
      //  pthread_detach(client_thread);
    }

    if (client_socket < 0) {
        perror("Accept failed");
        exit(EXIT_FAILURE);
    }
  
    close(client_socket);

    printf("Server closed.\n");
    return 0;
}
