#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080

void clear_screen() {
    printf("\033[H\033[J");
}

void display_menu() {
    clear_screen();
   printf("\n==========================================\n");
        printf("   Welcome to Railway Reservation System\n");
        printf("==========================================\n");
        printf("[1] Register\n");
        printf("[2] Login\n");
        printf("[3] Reserve Seat\n");
        printf("[4] Display Seat Availability Matrix\n");
        printf("[5] Check Reservation Status\n");
        printf("[6] Cancel reservation\n");
        printf("[7] Exit\n");
    printf("Choose an option: ");
}

// Function to communicate with server
void communicate_with_server(int sock) {
    char buffer[1024] = {0};
    char message[1024];

    while (1) {
        printf("Client: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = 0;  // Remove newline

        send(sock, message, strlen(message), 0);

        if (strcmp(message, "exit") == 0) {
            break;
        }

        read(sock, buffer, 1024);
        printf("Server: %s\n", buffer);
        memset(buffer, 0, sizeof(buffer));
    }
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation error.\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("Invalid address.\n");
        return -1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connection failed.\n");
        return -1;
    }

    printf("Connected to the server.\n");
     char username[50] = {0};
    char password[50] = {0};
    int option;
    char buffer[1024] = {0};
    char name[50]={0};
    int age,coach_num,pnr;
    //communicate_with_server(sock);
    do{
        display_menu();
        scanf("%d", &option);
        getchar(); // To consume the newline character

        switch (option) {
            case 1: // Register
                printf("Enter username: ");
                fgets(username, sizeof(username), stdin);
                username[strcspn(username, "\n")] = 0; // Remove newline

                printf("Enter password: ");
                fgets(password, sizeof(password), stdin);
                password[strcspn(password, "\n")] = 0; // Remove newline

                snprintf(buffer, sizeof(buffer), "REGISTER %s %s", username, password);
                send(sock, buffer, strlen(buffer), 0);
                break;

            case 2: // Login
                printf("Enter username: ");
                fgets(username, sizeof(username), stdin);
                username[strcspn(username, "\n")] = 0; // Remove newline

                printf("Enter password: ");
                fgets(password, sizeof(password), stdin);
                password[strcspn(password, "\n")] = 0; // Remove newline

                snprintf(buffer, sizeof(buffer), "LOGIN %s %s", username, password);
                send(sock, buffer, strlen(buffer), 0);
                break;

case 4: // Display Seats
                printf("Enter coach number(1-3): ");
                scanf("%d",&coach_num);
                getchar();
                coach_num--;
                snprintf(buffer, sizeof(buffer), "DISPLAY_SEATS %d",coach_num);
                send(sock, buffer, strlen(buffer), 0);
                break;

            case 3: // Book Seat
               printf("Enter Passenger Name: ");
               scanf("%s", name);
               printf("Enter Age: ");
               scanf("%d", &age);
               printf("Choose a coach(1-3):");
               scanf("%d", &coach_num);
                getchar(); // To consume the newline character
                coach_num--;
                snprintf(buffer, sizeof(buffer), "BOOK %s %s %d %d", username,name,age,coach_num);
                send(sock, buffer, strlen(buffer), 0);
                break;

            case 6: // Cancel Reservation
                printf("Enter your PNR to cancel: ");
                scanf("%d", &pnr);
                getchar(); // To consume the newline character
                snprintf(buffer, sizeof(buffer), "CANCEL %s %d", username, pnr);
                send(sock, buffer, strlen(buffer), 0);
                break;

            case 5: // View Reservation Status
            printf("Enter your PNR to view reservation: ");
            scanf("%d", &pnr);
            getchar();
                snprintf(buffer, sizeof(buffer), "RES_STATUS %s %d", username,pnr);
                send(sock, buffer, strlen(buffer), 0);
                break;
                

            case 7: // Exit
             printf("Thank you for using Railway Reservation System!!\n");
                snprintf(buffer, sizeof(buffer), "EXIT");
                send(sock, buffer, strlen(buffer), 0);
                close(sock);
                return 0;


            default:
                printf("Invalid option, please try again.\n");
                continue;
        }

        // Receive response from server
        int valread = read(sock, buffer, sizeof(buffer) - 1);
        buffer[valread] = '\0'; // Null-terminate the response
        printf("Server response: %s\n", buffer);

       if(option!=7){
       printf("\nPress any key to continue...");
       getchar();
    }
    }while(option!=7);

    close(sock);
    return 0;
}
