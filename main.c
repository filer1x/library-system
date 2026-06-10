#define _CRT_SECURE_NO_WARNINGS

#include "library_core.h"
#include "localization.h"

#include "raylib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int encodeUtf8(int codepoint, char *buf, int bufLen)
{
    if (codepoint <= 0x7F) {
        if (bufLen < 1) return 0;
        buf[0] = (char)codepoint;
        return 1;
    } else if (codepoint <= 0x7FF) {
        if (bufLen < 2) return 0;
        buf[0] = (char)(0xC0 | (codepoint >> 6));
        buf[1] = (char)(0x80 | (codepoint & 0x3F));
        return 2;
    } else if (codepoint <= 0xFFFF) {
        if (bufLen < 3) return 0;
        buf[0] = (char)(0xE0 | (codepoint >> 12));
        buf[1] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        buf[2] = (char)(0x80 | (codepoint & 0x3F));
        return 3;
    } else if (codepoint <= 0x10FFFF) {
        if (bufLen < 4) return 0;
        buf[0] = (char)(0xF0 | (codepoint >> 18));
        buf[1] = (char)(0x80 | ((codepoint >> 12) & 0x3F));
        buf[2] = (char)(0x80 | ((codepoint >> 6)  & 0x3F));
        buf[3] = (char)(0x80 | (codepoint & 0x3F));
        return 4;
    }
    return 0;
}

static void removeLastUtf8Char(char *str)
{
    int len = (int)strlen(str);
    if (len == 0) return;

    len--;
    while (len > 0 && ((unsigned char)str[len] & 0xC0) == 0x80) {
        len--;
    }
    str[len] = '\0';
}

static void appendCodepoint(char *dst, int maxBytes, int codepoint)
{
    char encoded[4];
    int encLen = encodeUtf8(codepoint, encoded, 4);
    int curLen = (int)strlen(dst);

    if (encLen > 0 && curLen + encLen < maxBytes) {
        memcpy(dst + curLen, encoded, encLen);
        dst[curLen + encLen] = '\0';
    }
}

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 720

#define SIDEBAR_WIDTH 280
#define SIDEBAR_COLOR GetColor(0x181825FF)

#define SIDEBAR_BUTTON_WIDTH 240
#define SIDEBAR_BUTTON_HEIGHT 50
#define SIDEBAR_BUTTON_START_Y 120
#define SIDEBAR_BUTTON_GAP 20
#define SIDEBAR_BUTTON_X 20

#define CONTENT_X 280
#define CONTENT_WIDTH (WINDOW_WIDTH - SIDEBAR_WIDTH)
#define CONTENT_PADDING 40

#define LIST_X (CONTENT_X + CONTENT_PADDING)
#define LIST_Y 180
#define ROW_HEIGHT 24
#define MAX_INPUT_CHARS 99

#define COLOR_BACKGROUND GetColor(0x1E1E2EFF)
#define COLOR_SIDEBAR SIDEBAR_COLOR
#define COLOR_PANEL GetColor(0x313244FF)
#define COLOR_BUTTON GetColor(0x89B4FAFF)
#define COLOR_BUTTON_HOVER GetColor(0x74C7ECFF)
#define COLOR_TEXT WHITE
#define COLOR_TEXT_DIM GetColor(0x9399B2FF)
#define COLOR_BUTTON_TEXT GetColor(0x11111BFF)
#define COLOR_ACCENT GetColor(0xF38BA8FF)
#define COLOR_SUCCESS GetColor(0xA6E3A1FF)
#define COLOR_INPUT_BG GetColor(0x45475AFF)
#define COLOR_INPUT_ACTIVE GetColor(0x585B70FF)

typedef enum ViewMode {
    VIEW_LOGIN,
    VIEW_REGISTER,
    VIEW_HOME,
    VIEW_ALL,
    VIEW_SEARCH_RESULTS,
    VIEW_ADD_BOOK,
    VIEW_USER_ACTIONS
} ViewMode;

typedef enum ActiveInput {
    INPUT_NONE,
    INPUT_TITLE,
    INPUT_AUTHOR,
    INPUT_YEAR,
    INPUT_SEARCH,
    INPUT_USERNAME,
    INPUT_PASSWORD,
    INPUT_CONFIRM_PASSWORD,
    INPUT_BOOK_ID
} ActiveInput;

static int buttonWasClicked(Rectangle button)
{
    Vector2 mouse = GetMousePosition();

    return CheckCollisionPointRec(mouse, button) &&
           IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
}

static void drawButton(Rectangle button, const char *label, Font font)
{
    Vector2 mouse = GetMousePosition();
    int hovered = CheckCollisionPointRec(mouse, button);
    Color fill = hovered ? COLOR_BUTTON_HOVER : COLOR_BUTTON;

    DrawRectangleRounded(button, 0.2f, 8, fill);

    Vector2 textSize = MeasureTextEx(font, label, 20, 1.0f);
    Vector2 textPos;
    textPos.x = button.x + (button.width - textSize.x) / 2.0f;
    textPos.y = button.y + (button.height - textSize.y) / 2.0f;

    DrawTextEx(font, label, textPos, 20, 1.0f, COLOR_BUTTON_TEXT);
}

static void drawInputField(Rectangle field, const char *label, const char *value, int isActive, Font font)
{
    Color bgColor = isActive ? COLOR_INPUT_ACTIVE : COLOR_INPUT_BG;
    Color borderColor = isActive ? COLOR_BUTTON : COLOR_TEXT_DIM;

    DrawRectangleRounded(field, 0.15f, 8, bgColor);
    DrawRectangleRoundedLines(field, 0.15f, 8, borderColor);

    Vector2 labelPos = {field.x, field.y - 25};
    DrawTextEx(font, label, labelPos, 18, 1, COLOR_TEXT);

    Vector2 valuePos = {field.x + 10, field.y + 10};
    DrawTextEx(font, value, valuePos, 20, 1, COLOR_TEXT);

    if (isActive) {
        int textWidth = (int)MeasureTextEx(font, value, 20, 1).x;
        float cursorX = field.x + 10 + textWidth;
        float cursorY = field.y + 8;

        if (((int)(GetTime() * 2)) % 2 == 0) {
            DrawRectangle((int)cursorX, (int)cursorY, 2, 24, COLOR_TEXT);
        }
    }
}

static int nextBookId(void)
{
    int i;
    int highestId = 0;

    for (i = 0; i < getBookCount(); i++) {
        const Book *book = getBookAt(i);

        if (book != NULL && book->id > highestId) {
            highestId = book->id;
        }
    }

    return highestId + 1;
}

static void drawBookList(int x, int y, int maxRows, Font font, Language lang, User currentUser, int isLoggedIn)
{
    int i;
    int count = getBookCount();

    if (count == 0) {
        const char *emptyText = localize(lang, TEXT_LIBRARY_EMPTY);
        Vector2 pos = {(float)x, (float)y};
        DrawTextEx(font, emptyText, pos, 20, 1, COLOR_TEXT_DIM);
        return;
    }

    Vector2 headerPos1 = {(float)x, (float)y};
    Vector2 headerPos2 = {(float)(x + 70), (float)y};
    Vector2 headerPos3 = {(float)(x + 390), (float)y};
    Vector2 headerPos4 = {(float)(x + 680), (float)y};
    Vector2 headerPos5 = {(float)(x + 780), (float)y};

    DrawTextEx(font, "ID", headerPos1, 20, 1, COLOR_BUTTON);
    DrawTextEx(font, localize(lang, TEXT_HEADER_TITLE), headerPos2, 20, 1, COLOR_BUTTON);
    DrawTextEx(font, localize(lang, TEXT_HEADER_AUTHOR), headerPos3, 20, 1, COLOR_BUTTON);
    DrawTextEx(font, localize(lang, TEXT_HEADER_YEAR), headerPos4, 20, 1, COLOR_BUTTON);
    DrawTextEx(font, "Статус", headerPos5, 20, 1, COLOR_BUTTON);

    DrawRectangle(x, y + 28, 900, 2, COLOR_TEXT_DIM);

    for (i = 0; i < count && i < maxRows; i++) {
        const Book *book = getBookAt(i);
        int rowY = y + 42 + (i * ROW_HEIGHT);
        char idText[16];
        char yearText[16];
        char statusText[32];

        if (book == NULL) {
            continue;
        }

        snprintf(idText, sizeof(idText), "%d", book->id);
        snprintf(yearText, sizeof(yearText), "%d", book->year);

        if (book->isReserved) {
            snprintf(statusText, sizeof(statusText), "Заброньовано");
        } else {
            snprintf(statusText, sizeof(statusText), "Вільно");
        }

        Vector2 pos1 = {(float)x, (float)rowY};
        Vector2 pos2 = {(float)(x + 70), (float)rowY};
        Vector2 pos3 = {(float)(x + 390), (float)rowY};
        Vector2 pos4 = {(float)(x + 680), (float)rowY};
        Vector2 pos5 = {(float)(x + 780), (float)rowY};

        DrawTextEx(font, idText, pos1, 18, 1, COLOR_TEXT);
        DrawTextEx(font, book->title, pos2, 18, 1, COLOR_TEXT);
        DrawTextEx(font, book->author, pos3, 18, 1, COLOR_TEXT);
        DrawTextEx(font, yearText, pos4, 18, 1, COLOR_TEXT);
        DrawTextEx(font, statusText, pos5, 18, 1, book->isReserved ? COLOR_ACCENT : COLOR_SUCCESS);
    }

    if (count > maxRows) {
        Vector2 morePos = {(float)x, (float)(y + 42 + (maxRows * ROW_HEIGHT))};
        DrawTextEx(font, "...", morePos, 18, 1, COLOR_TEXT_DIM);
    }
}

static void drawSearchResults(const Book results[], int count, int x, int y,
                              const char *query, Font font)
{
    int i;
    char searchLabel[] = "Пошук за назвою:";
    Rectangle searchField = {(float)x, (float)(y - 60), 500, 45};

    DrawRectangleRounded(searchField, 0.15f, 8, COLOR_INPUT_BG);
    DrawRectangleRoundedLines(searchField, 0.15f, 8, COLOR_BUTTON);

    Vector2 labelPos = {(float)x, (float)(y - 85)};
    DrawTextEx(font, searchLabel, labelPos, 18, 1, COLOR_TEXT);

    Vector2 queryPos = {(float)(x + 10), (float)(y - 50)};
    DrawTextEx(font, query, queryPos, 20, 1, COLOR_TEXT);

    {
        int textWidth = (int)MeasureTextEx(font, query, 20, 1).x;
        float cursorX = searchField.x + 10 + textWidth;
        float cursorY = searchField.y + 8;
        if (((int)(GetTime() * 2)) % 2 == 0) {
            DrawRectangle((int)cursorX, (int)cursorY, 2, 24, COLOR_TEXT);
        }
    }

    if (strlen(query) == 0) {
        Vector2 hintPos = {(float)x, (float)y};
        DrawTextEx(font, "Введіть текст для пошуку...", hintPos, 20, 1, COLOR_TEXT_DIM);
        return;
    }

    if (count == 0) {
        Vector2 pos = {(float)x, (float)y};
        DrawTextEx(font, "Не знайдено результатів", pos, 20, 1, COLOR_TEXT_DIM);
        return;
    }

    char headerText[128];
    snprintf(headerText, sizeof(headerText), "Знайдено результатів: %d", count);
    Vector2 headerPos = {(float)x, (float)y};
    DrawTextEx(font, headerText, headerPos, 20, 1, COLOR_BUTTON);

    for (i = 0; i < count && i < 14; i++) {
        char line[260];

        snprintf(line,
                 sizeof(line),
                 "%d | %s | %s | %d",
                 results[i].id,
                 results[i].title,
                 results[i].author,
                 results[i].year);

        Vector2 linePos = {(float)x, (float)(y + 34 + (i * ROW_HEIGHT))};
        DrawTextEx(font, line, linePos, 18, 1, COLOR_TEXT);
    }
}

int main(void)
{
    Rectangle addButton = {
        SIDEBAR_BUTTON_X,
        SIDEBAR_BUTTON_START_Y,
        SIDEBAR_BUTTON_WIDTH,
        SIDEBAR_BUTTON_HEIGHT
    };
    Rectangle searchButton = {
        SIDEBAR_BUTTON_X,
        SIDEBAR_BUTTON_START_Y + (SIDEBAR_BUTTON_HEIGHT + SIDEBAR_BUTTON_GAP),
        SIDEBAR_BUTTON_WIDTH,
        SIDEBAR_BUTTON_HEIGHT
    };
    Rectangle viewAllButton = {
        SIDEBAR_BUTTON_X,
        SIDEBAR_BUTTON_START_Y + 2 * (SIDEBAR_BUTTON_HEIGHT + SIDEBAR_BUTTON_GAP),
        SIDEBAR_BUTTON_WIDTH,
        SIDEBAR_BUTTON_HEIGHT
    };
    Rectangle exitButton = {
        SIDEBAR_BUTTON_X,
        SIDEBAR_BUTTON_START_Y + 3 * (SIDEBAR_BUTTON_HEIGHT + SIDEBAR_BUTTON_GAP),
        SIDEBAR_BUTTON_WIDTH,
        SIDEBAR_BUTTON_HEIGHT
    };

    Book searchResults[MAX_BOOKS];
    int searchCount = 0;
    const int maxVisibleRows = 16;
    ViewMode currentView = VIEW_LOGIN;
    Language currentLang = LANGUAGE_UKRAINIAN;

    char statusText[160];
    char titleInput[MAX_INPUT_CHARS + 1] = {0};
    char authorInput[MAX_INPUT_CHARS + 1] = {0};
    char yearInput[MAX_INPUT_CHARS + 1] = {0};
    char searchInput[MAX_INPUT_CHARS + 1] = {0};
    char usernameInput[MAX_INPUT_CHARS + 1] = {0};
    char passwordInput[MAX_INPUT_CHARS + 1] = {0};
    char confirmPasswordInput[MAX_INPUT_CHARS + 1] = {0};
    char bookIdInput[MAX_INPUT_CHARS + 1] = {0};
    ActiveInput activeInput = INPUT_NONE;

    User currentUser;
    int isLoggedIn = 0;

    Rectangle userActionsButton = {
        SIDEBAR_BUTTON_X,
        SIDEBAR_BUTTON_START_Y + 4 * (SIDEBAR_BUTTON_HEIGHT + SIDEBAR_BUTTON_GAP),
        SIDEBAR_BUTTON_WIDTH,
        SIDEBAR_BUTTON_HEIGHT
    };

    Font customFont;

    snprintf(statusText, sizeof(statusText), "");

    if (!loadBooks()) {
        initLibrary();
    }

    loadUsers();

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Library App");
    SetTargetFPS(60);

int codepoints[400];
int count = 0;

for (int i = 32; i <= 126; i++)
    codepoints[count++] = i;

for (int i = 0x0400; i <= 0x052F; i++)
    codepoints[count++] = i;

customFont = LoadFontEx(
    "assets/DejaVuSans.ttf",
    32,
    codepoints,
    count
);

  printf("font id = %u\n", customFont.texture.id);
  printf("cwd = %s\n", GetWorkingDirectory());

   if (customFont.texture.id == 0) { 
    printf("FONT LOAD FAILED\n");
    customFont = GetFontDefault();
   } else {
    printf("FONT LOADED OK\n");
   }

SetTextureFilter(customFont.texture, TEXTURE_FILTER_BILINEAR);

    while (!WindowShouldClose()) {
        if (currentView == VIEW_LOGIN) {
            Rectangle usernameField = {CONTENT_X + 100, 200, 400, 45};
            Rectangle passwordField = {CONTENT_X + 100, 280, 400, 45};
            Rectangle loginButton = {CONTENT_X + 100, 360, 180, 50};
            Rectangle registerButton = {CONTENT_X + 300, 360, 180, 50};

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                Vector2 mouse = GetMousePosition();

                if (CheckCollisionPointRec(mouse, usernameField)) {
                    activeInput = INPUT_USERNAME;
                } else if (CheckCollisionPointRec(mouse, passwordField)) {
                    activeInput = INPUT_PASSWORD;
                } else {
                    activeInput = INPUT_NONE;
                }
            }

            int key = GetCharPressed();
            while (key > 0) {
                if (activeInput == INPUT_USERNAME) {
                    appendCodepoint(usernameInput, MAX_INPUT_CHARS + 1, key);
                } else if (activeInput == INPUT_PASSWORD) {
                    appendCodepoint(passwordInput, MAX_INPUT_CHARS + 1, key);
                }
                key = GetCharPressed();
            }

            if (IsKeyPressed(KEY_BACKSPACE)) {
                if (activeInput == INPUT_USERNAME) {
                    removeLastUtf8Char(usernameInput);
                } else if (activeInput == INPUT_PASSWORD) {
                    removeLastUtf8Char(passwordInput);
                }
            }

            if (buttonWasClicked(loginButton)) {
                if (authenticateUser(usernameInput, passwordInput, &currentUser)) {
                    isLoggedIn = 1;
                    currentView = VIEW_HOME;
                    snprintf(statusText, sizeof(statusText), "Вітаємо, %s!", currentUser.username);
                } else {
                    snprintf(statusText, sizeof(statusText), "Невірний логін або пароль");
                }
            }

            if (buttonWasClicked(registerButton)) {
                currentView = VIEW_REGISTER;
                usernameInput[0] = '\0';
                passwordInput[0] = '\0';
                confirmPasswordInput[0] = '\0';
                snprintf(statusText, sizeof(statusText), "");
            }

            BeginDrawing();
            ClearBackground(COLOR_BACKGROUND);

            Vector2 titlePos = {CONTENT_X + 100, 80};
            DrawTextEx(customFont, u8"Вхід в систему", titlePos, 32, 1, COLOR_TEXT);

            drawInputField(usernameField, u8"Логін:", usernameInput, activeInput == INPUT_USERNAME, customFont);
            drawInputField(passwordField, u8"Пароль:", passwordInput, activeInput == INPUT_PASSWORD, customFont);

            drawButton(loginButton, u8"Увійти", customFont);
            drawButton(registerButton, u8"Реєстрація", customFont);

            Vector2 statusPos = {CONTENT_X + 100, 440};
            DrawTextEx(customFont, statusText, statusPos, 18, 1, COLOR_ACCENT);

            EndDrawing();
            continue;
        }

        if (currentView == VIEW_REGISTER) {
            Rectangle usernameField = {CONTENT_X + 100, 180, 400, 45};
            Rectangle passwordField = {CONTENT_X + 100, 260, 400, 45};
            Rectangle confirmPasswordField = {CONTENT_X + 100, 340, 400, 45};
            Rectangle registerButton = {CONTENT_X + 100, 420, 180, 50};
            Rectangle backButton = {CONTENT_X + 300, 420, 180, 50};

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                Vector2 mouse = GetMousePosition();

                if (CheckCollisionPointRec(mouse, usernameField)) {
                    activeInput = INPUT_USERNAME;
                } else if (CheckCollisionPointRec(mouse, passwordField)) {
                    activeInput = INPUT_PASSWORD;
                } else if (CheckCollisionPointRec(mouse, confirmPasswordField)) {
                    activeInput = INPUT_CONFIRM_PASSWORD;
                } else {
                    activeInput = INPUT_NONE;
                }
            }

            int key = GetCharPressed();
            while (key > 0) {
                if (activeInput == INPUT_USERNAME) {
                    appendCodepoint(usernameInput, MAX_INPUT_CHARS + 1, key);
                } else if (activeInput == INPUT_PASSWORD) {
                    appendCodepoint(passwordInput, MAX_INPUT_CHARS + 1, key);
                } else if (activeInput == INPUT_CONFIRM_PASSWORD) {
                    appendCodepoint(confirmPasswordInput, MAX_INPUT_CHARS + 1, key);
                }
                key = GetCharPressed();
            }

            if (IsKeyPressed(KEY_BACKSPACE)) {
                if (activeInput == INPUT_USERNAME) {
                    removeLastUtf8Char(usernameInput);
                } else if (activeInput == INPUT_PASSWORD) {
                    removeLastUtf8Char(passwordInput);
                } else if (activeInput == INPUT_CONFIRM_PASSWORD) {
                    removeLastUtf8Char(confirmPasswordInput);
                }
            }

            if (buttonWasClicked(registerButton)) {
                if (strlen(usernameInput) == 0 || strlen(passwordInput) == 0) {
                    snprintf(statusText, sizeof(statusText), "Заповніть усі поля!");
                } else if (strcmp(passwordInput, confirmPasswordInput) != 0) {
                    snprintf(statusText, sizeof(statusText), "Паролі не співпадають!");
                } else {
                    User newUser;
                    strncpy(newUser.username, usernameInput, MAX_USERNAME - 1);
                    newUser.username[MAX_USERNAME - 1] = '\0';
                    strncpy(newUser.password, passwordInput, MAX_PASSWORD - 1);
                    newUser.password[MAX_PASSWORD - 1] = '\0';
                    newUser.role = ROLE_USER;

                    if (addUser(&newUser)) {
                        snprintf(statusText, sizeof(statusText), "Акаунт створено! Тепер увійдіть.");
                    } else {
                        snprintf(statusText, sizeof(statusText), "Користувач з таким іменем вже існує!");
                    }
                }
            }

            if (buttonWasClicked(backButton)) {
                currentView = VIEW_LOGIN;
                usernameInput[0] = '\0';
                passwordInput[0] = '\0';
                confirmPasswordInput[0] = '\0';
                snprintf(statusText, sizeof(statusText), "");
            }

            BeginDrawing();
            ClearBackground(COLOR_BACKGROUND);

            Vector2 titlePos = {CONTENT_X + 100, 80};
            DrawTextEx(customFont, u8"Реєстрація", titlePos, 32, 1, COLOR_TEXT);

            drawInputField(usernameField, u8"Логін:", usernameInput, activeInput == INPUT_USERNAME, customFont);
            drawInputField(passwordField, u8"Пароль:", passwordInput, activeInput == INPUT_PASSWORD, customFont);
            drawInputField(confirmPasswordField, u8"Підтвердьте пароль:", confirmPasswordInput, activeInput == INPUT_CONFIRM_PASSWORD, customFont);

            drawButton(registerButton, u8"Зареєструватися", customFont);
            drawButton(backButton, u8"Назад", customFont);

            Vector2 statusPos = {CONTENT_X + 100, 500};
            DrawTextEx(customFont, statusText, statusPos, 18, 1, COLOR_ACCENT);

            EndDrawing();
            continue;
        }

        if (currentView != VIEW_ADD_BOOK) {
            if (buttonWasClicked(addButton) && currentUser.role == ROLE_ADMIN) {
                currentView = VIEW_ADD_BOOK;
                titleInput[0] = '\0';
                authorInput[0] = '\0';
                yearInput[0] = '\0';
                activeInput = INPUT_NONE;
                snprintf(statusText, sizeof(statusText), "%s", localize(currentLang, TEXT_MENU_ADD_BOOK));
            }

            if (buttonWasClicked(searchButton)) {
                searchCount = 0;
                searchInput[0] = '\0';
                activeInput = INPUT_SEARCH;
                currentView = VIEW_SEARCH_RESULTS;
                snprintf(statusText, sizeof(statusText), "Пошук книги");
            }

            if (buttonWasClicked(viewAllButton)) {
                currentView = VIEW_ALL;
                snprintf(statusText, sizeof(statusText), "%s %d", localize(currentLang, TEXT_MENU_SHOW_ALL), getBookCount());
            }

            if (buttonWasClicked(userActionsButton) && isLoggedIn) {
                currentView = VIEW_USER_ACTIONS;
                bookIdInput[0] = '\0';
                snprintf(statusText, sizeof(statusText), "Дії з книгами");
            }

            if (buttonWasClicked(exitButton)) {
                break;
            }
        }

        if (currentView == VIEW_ADD_BOOK) {
            Rectangle titleField = {CONTENT_X + 100, 180, 500, 45};
            Rectangle authorField = {CONTENT_X + 100, 270, 500, 45};
            Rectangle yearField = {CONTENT_X + 100, 360, 200, 45};
            Rectangle saveButton = {CONTENT_X + 100, 450, 180, 50};
            Rectangle cancelButton = {CONTENT_X + 300, 450, 180, 50};

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                Vector2 mouse = GetMousePosition();

                if (CheckCollisionPointRec(mouse, titleField)) {
                    activeInput = INPUT_TITLE;
                } else if (CheckCollisionPointRec(mouse, authorField)) {
                    activeInput = INPUT_AUTHOR;
                } else if (CheckCollisionPointRec(mouse, yearField)) {
                    activeInput = INPUT_YEAR;
                } else {
                    activeInput = INPUT_NONE;
                }
            }

            int key = GetCharPressed();
            while (key > 0) {
                if (activeInput == INPUT_TITLE) {
                    appendCodepoint(titleInput, MAX_INPUT_CHARS + 1, key);
                } else if (activeInput == INPUT_AUTHOR) {
                    appendCodepoint(authorInput, MAX_INPUT_CHARS + 1, key);
                } else if (activeInput == INPUT_YEAR) {
                    if (key >= '0' && key <= '9') {
                        appendCodepoint(yearInput, MAX_INPUT_CHARS + 1, key);
                    }
                }
                key = GetCharPressed();
            }

            if (IsKeyPressed(KEY_BACKSPACE)) {
                if (activeInput == INPUT_TITLE) {
                    removeLastUtf8Char(titleInput);
                } else if (activeInput == INPUT_AUTHOR) {
                    removeLastUtf8Char(authorInput);
                } else if (activeInput == INPUT_YEAR) {
                    removeLastUtf8Char(yearInput);
                }
            }

            if (buttonWasClicked(saveButton)) {
                if (strlen(titleInput) > 0 && strlen(authorInput) > 0 && strlen(yearInput) > 0) {
                    Book newBook;
                    newBook.id = nextBookId();
                    strncpy(newBook.title, titleInput, MAX_TITLE - 1);
                    newBook.title[MAX_TITLE - 1] = '\0';
                    strncpy(newBook.author, authorInput, MAX_AUTHOR - 1);
                    newBook.author[MAX_AUTHOR - 1] = '\0';
                    newBook.year = atoi(yearInput);
                    newBook.isReserved = 0;
                    newBook.reservedBy[0] = '\0';

                    if (addBook(&newBook)) {
                        snprintf(statusText, sizeof(statusText), "%s ID: %d", localize(currentLang, TEXT_BOOK_ADDED), newBook.id);
                        currentView = VIEW_ALL;
                    } else {
                        snprintf(statusText, sizeof(statusText), "%s", localize(currentLang, TEXT_BOOK_ADD_FAILED));
                    }
                } else {
                    snprintf(statusText, sizeof(statusText), "Заповніть усі поля!");
                }
            }

            if (buttonWasClicked(cancelButton)) {
                currentView = VIEW_HOME;
                snprintf(statusText, sizeof(statusText), "Скасовано");
            }
        }

        if (currentView == VIEW_SEARCH_RESULTS) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                Vector2 mouse = GetMousePosition();
                Rectangle searchField = {(float)LIST_X, (float)(LIST_Y - 60), 500, 45};
                if (CheckCollisionPointRec(mouse, searchField)) {
                    activeInput = INPUT_SEARCH;
                } else if (mouse.x >= CONTENT_X) {
                    activeInput = INPUT_NONE;
                }
            }

            if (activeInput == INPUT_SEARCH) {
                int key = GetCharPressed();
                while (key > 0) {
                    appendCodepoint(searchInput, MAX_INPUT_CHARS + 1, key);
                    key = GetCharPressed();
                }
                if (IsKeyPressed(KEY_BACKSPACE)) {
                    removeLastUtf8Char(searchInput);
                }
                searchCount = searchBookByTitle(searchInput, searchResults, MAX_BOOKS);
            }
        }

        BeginDrawing();
        ClearBackground(COLOR_BACKGROUND);

        DrawRectangle(0, 0, SIDEBAR_WIDTH, WINDOW_HEIGHT, COLOR_SIDEBAR);

        Vector2 sidebarTitlePos = {30, 40};
        DrawTextEx(customFont, localize(currentLang, TEXT_APP_TITLE), sidebarTitlePos, 28, 1, COLOR_TEXT);

        if (isLoggedIn && currentUser.role == ROLE_ADMIN) {
            drawButton(addButton, localize(currentLang, TEXT_MENU_ADD_BOOK), customFont);
        }
        drawButton(searchButton, "Пошук", customFont);
        drawButton(viewAllButton, localize(currentLang, TEXT_MENU_SHOW_ALL), customFont);
        drawButton(userActionsButton, u8"Дії з книгами", customFont);
        drawButton(exitButton, u8"Вийти з бібліотеки", customFont);

        Vector2 contentTitlePos = {CONTENT_X + CONTENT_PADDING, 40};
        DrawTextEx(customFont, statusText, contentTitlePos, 24, 1, COLOR_TEXT);

        if (currentView == VIEW_ALL) {
            drawBookList(LIST_X, LIST_Y, maxVisibleRows, customFont, currentLang, currentUser, isLoggedIn);
        } else if (currentView == VIEW_SEARCH_RESULTS) {
            drawSearchResults(searchResults, searchCount, LIST_X, LIST_Y, searchInput, customFont);
        } else if (currentView == VIEW_ADD_BOOK) {
            Rectangle titleField = {CONTENT_X + 100, 180, 500, 45};
            Rectangle authorField = {CONTENT_X + 100, 270, 500, 45};
            Rectangle yearField = {CONTENT_X + 100, 360, 200, 45};
            Rectangle saveButton = {CONTENT_X + 100, 450, 180, 50};
            Rectangle cancelButton = {CONTENT_X + 300, 450, 180, 50};

            Vector2 headerPos = {CONTENT_X + 100, 120};
            DrawTextEx(customFont, u8"Додати нову книгу", headerPos, 26, 1, COLOR_TEXT);

            drawInputField(titleField, localize(currentLang, TEXT_PROMPT_TITLE), titleInput, activeInput == INPUT_TITLE, customFont);
            drawInputField(authorField, localize(currentLang, TEXT_PROMPT_AUTHOR), authorInput, activeInput == INPUT_AUTHOR, customFont);
            drawInputField(yearField, localize(currentLang, TEXT_PROMPT_YEAR), yearInput, activeInput == INPUT_YEAR, customFont);

            drawButton(saveButton, u8"Зберегти", customFont);
            drawButton(cancelButton, u8"Скасувати", customFont);
        }

        if (currentView == VIEW_USER_ACTIONS) {
            Rectangle bookIdField = {LIST_X, LIST_Y + 30, 200, 45};
            Rectangle reserveButton = {LIST_X + 220, LIST_Y + 30, 200, 45};
            Rectangle returnButton = {LIST_X + 440, LIST_Y + 30, 200, 45};
            Rectangle backButton = {LIST_X, LIST_Y + 100, 180, 45};

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                Vector2 mouse = GetMousePosition();
                if (CheckCollisionPointRec(mouse, bookIdField)) {
                    activeInput = INPUT_BOOK_ID;
                } else {
                    activeInput = INPUT_NONE;
                }
            }

            int key = GetCharPressed();
            while (key > 0) {
                if (activeInput == INPUT_BOOK_ID && key >= '0' && key <= '9') {
                    appendCodepoint(bookIdInput, MAX_INPUT_CHARS + 1, key);
                }
                key = GetCharPressed();
            }

            if (IsKeyPressed(KEY_BACKSPACE) && activeInput == INPUT_BOOK_ID) {
                removeLastUtf8Char(bookIdInput);
            }

            if (buttonWasClicked(reserveButton)) {
                int bookId = atoi(bookIdInput);
                if (reserveBook(bookId, currentUser.username)) {
                    snprintf(statusText, sizeof(statusText), "Книгу заброньовано!");
                } else {
                    snprintf(statusText, sizeof(statusText), "Не вдалося забронювати книгу");
                }
            }

            if (buttonWasClicked(returnButton)) {
                int bookId = atoi(bookIdInput);
                if (returnBook(bookId, currentUser.username, currentUser.role)) {
                    snprintf(statusText, sizeof(statusText), "Книгу повернено!");
                } else {
                    snprintf(statusText, sizeof(statusText), "Не вдалося повернути книгу");
                }
            }

            if (buttonWasClicked(backButton)) {
                currentView = VIEW_HOME;
                bookIdInput[0] = '\0';
                snprintf(statusText, sizeof(statusText), "Вітаємо, %s!", currentUser.username);
            }

            Vector2 labelPos = {LIST_X, LIST_Y};
            DrawTextEx(customFont, u8"ID книги:", labelPos, 18, 1, COLOR_TEXT);
            drawInputField(bookIdField, "", bookIdInput, activeInput == INPUT_BOOK_ID, customFont);
            drawButton(reserveButton, u8"Забронювати", customFont);
            drawButton(returnButton, u8"Віддати", customFont);
            drawButton(backButton, u8"Назад", customFont);
        }

        EndDrawing();
    }

    saveBooks();
    UnloadFont(customFont);
    CloseWindow();

    return 0;
}
