#define _CRT_SECURE_NO_WARNINGS

#include "library_core.h"

#include <stdio.h>
#include <string.h>

static Book library[MAX_BOOKS];
static int bookCount = 0;

#define MAX_USERS 50
static User users[MAX_USERS];
static int userCount = 0;

static int findBookIndexById(int id)
{
    int i;

    for (i = 0; i < bookCount; i++) {
        if (library[i].id == id) {
            return i;
        }
    }

    return -1;
}

void initLibrary(void)
{
    memset(library, 0, sizeof(library));
    bookCount = 0;
}

int getBookCount(void)
{
    return bookCount;
}

const Book *getBookAt(int index)
{
    if (index < 0 || index >= bookCount) {
        return NULL;
    }

    return &library[index];
}

int bookIdExists(int id)
{
    return findBookIndexById(id) != -1;
}

int addBook(const Book *book)
{
    if (book == NULL || bookCount >= MAX_BOOKS || bookIdExists(book->id)) {
        return 0;
    }

    library[bookCount] = *book;
    bookCount++;

    return saveBooks();
}

int searchBookByTitle(const char *title, Book results[], int maxResults)
{
    int i;
    int found = 0;

    if (title == NULL || results == NULL || maxResults <= 0) {
        return 0;
    }

    for (i = 0; i < bookCount && found < maxResults; i++) {
        if (strstr(library[i].title, title) != NULL) {
            results[found] = library[i];
            found++;
        }
    }

    return found;
}


int loadBooks(void)
{
    FILE *file;
    int loadedCount;

    file = fopen(DATA_FILE, "rb");

    if (file == NULL) {
        initLibrary();
        return 0;
    }

    if (fread(&loadedCount, sizeof(int), 1, file) != 1) {
        initLibrary();
        fclose(file);
        return 0;
    }

    if (loadedCount < 0 || loadedCount > MAX_BOOKS) {
        initLibrary();
        fclose(file);
        return 0;
    }

    if (fread(library, sizeof(Book), MAX_BOOKS, file) != MAX_BOOKS) {
        initLibrary();
        fclose(file);
        return 0;
    }

    bookCount = loadedCount;
    fclose(file);

    return 1;
}

int saveBooks(void)
{
    FILE *file;

    file = fopen(DATA_FILE, "wb");

    if (file == NULL) {
        return 0;
    }

    if (fwrite(&bookCount, sizeof(int), 1, file) != 1 ||
        fwrite(library, sizeof(Book), MAX_BOOKS, file) != MAX_BOOKS) {
        fclose(file);
        return 0;
    }

    fclose(file);
    return 1;
}

int reserveBook(int bookId, const char *username)
{
    int index = findBookIndexById(bookId);

    if (index == -1 || library[index].isReserved) {
        return 0;
    }

    library[index].isReserved = 1;
    strncpy(library[index].reservedBy, username, MAX_USERNAME - 1);
    library[index].reservedBy[MAX_USERNAME - 1] = '\0';

    return saveBooks();
}

int returnBook(int bookId, const char *username, UserRole role)
{
    int index = findBookIndexById(bookId);

    if (index == -1 || !library[index].isReserved) {
        return 0;
    }

    if (role != ROLE_ADMIN && strcmp(library[index].reservedBy, username) != 0) {
        return 0;
    }

    library[index].isReserved = 0;
    library[index].reservedBy[0] = '\0';

    return saveBooks();
}

int deleteBook(int bookId)
{
    int index = findBookIndexById(bookId);
    int i;

    if (index == -1) {
        return 0;
    }

    for (i = index; i < bookCount - 1; i++) {
        library[i] = library[i + 1];
    }

    bookCount--;
    return saveBooks();
}

static int findUserIndexByUsername(const char *username)
{
    int i;

    for (i = 0; i < userCount; i++) {
        if (strcmp(users[i].username, username) == 0) {
            return i;
        }
    }

    return -1;
}

void initUsers(void)
{
    memset(users, 0, sizeof(users));
    userCount = 0;
}

int authenticateUser(const char *username, const char *password, User *user)
{
    int index = findUserIndexByUsername(username);

    if (index == -1) {
        return 0;
    }

    if (strcmp(users[index].password, password) == 0) {
        if (user != NULL) {
            *user = users[index];
        }
        return 1;
    }

    return 0;
}

int addUser(const User *user)
{
    if (user == NULL || userCount >= MAX_USERS || findUserIndexByUsername(user->username) != -1) {
        return 0;
    }

    users[userCount] = *user;
    userCount++;

    return saveUsers();
}

int loadUsers(void)
{
    FILE *file;
    int loadedCount;

    file = fopen(USERS_FILE, "rb");

    if (file == NULL) {
        initUsers();
        
        User admin;
        strncpy(admin.username, "admin", MAX_USERNAME - 1);
        admin.username[MAX_USERNAME - 1] = '\0';
        strncpy(admin.password, "admin", MAX_PASSWORD - 1);
        admin.password[MAX_PASSWORD - 1] = '\0';
        admin.role = ROLE_ADMIN;
        addUser(&admin);
        
        return 0;
    }

    if (fread(&loadedCount, sizeof(int), 1, file) != 1) {
        initUsers();
        fclose(file);
        return 0;
    }

    if (loadedCount < 0 || loadedCount > MAX_USERS) {
        initUsers();
        fclose(file);
        return 0;
    }

    if (fread(users, sizeof(User), MAX_USERS, file) != MAX_USERS) {
        initUsers();
        fclose(file);
        return 0;
    }

    userCount = loadedCount;
    fclose(file);

    return 1;
}

int saveUsers(void)
{
    FILE *file;

    file = fopen(USERS_FILE, "wb");

    if (file == NULL) {
        return 0;
    }

    if (fwrite(&userCount, sizeof(int), 1, file) != 1 ||
        fwrite(users, sizeof(User), MAX_USERS, file) != MAX_USERS) {
        fclose(file);
        return 0;
    }

    fclose(file);
    return 1;
}
