#ifndef LIBRARY_CORE_H
#define LIBRARY_CORE_H

#define MAX_BOOKS 100
#define MAX_TITLE 100
#define MAX_AUTHOR 100
#define MAX_USERNAME 50
#define MAX_PASSWORD 50
#define DATA_FILE "data.bin"
#define USERS_FILE "users.bin"

typedef enum UserRole {
    ROLE_USER = 0,
    ROLE_ADMIN = 1
} UserRole;

typedef struct User {
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
    UserRole role;
} User;

typedef struct Book {
    int id;
    char title[MAX_TITLE];
    char author[MAX_AUTHOR];
    int year;
    int isReserved;
    char reservedBy[MAX_USERNAME];
} Book;

void initLibrary(void);
int getBookCount(void);
const Book *getBookAt(int index);
int bookIdExists(int id);
int addBook(const Book *book);
int searchBookByTitle(const char *title, Book results[], int maxResults);
int loadBooks(void);
int saveBooks(void);
int reserveBook(int bookId, const char *username);
int returnBook(int bookId, const char *username, UserRole role);
int deleteBook(int bookId);

void initUsers(void);
int authenticateUser(const char *username, const char *password, User *user);
int addUser(const User *user);
int loadUsers(void);
int saveUsers(void);

#endif
