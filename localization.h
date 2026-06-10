#ifndef LOCALIZATION_H
#define LOCALIZATION_H

typedef enum Language {
    LANGUAGE_ENGLISH = 0,
    LANGUAGE_UKRAINIAN = 1,
    LANGUAGE_COUNT = 2
} Language;

typedef enum TextKey {
    TEXT_APP_TITLE,
    TEXT_MENU_ADD_BOOK,
    TEXT_MENU_SHOW_ALL,
    TEXT_MENU_EXIT,
    TEXT_PROMPT_TITLE,
    TEXT_PROMPT_AUTHOR,
    TEXT_PROMPT_YEAR,
    TEXT_LIBRARY_EMPTY,
    TEXT_BOOK_ADDED,
    TEXT_BOOK_ADD_FAILED,
    TEXT_HEADER_TITLE,
    TEXT_HEADER_AUTHOR,
    TEXT_HEADER_YEAR,
    TEXT_COUNT
} TextKey;

typedef struct LocalizationEntry {
    TextKey key;
    const char *translations[LANGUAGE_COUNT];
} LocalizationEntry;

static const LocalizationEntry localizationDictionary[TEXT_COUNT] = {
    {TEXT_APP_TITLE, {"Library App", "Бібліотека"}},
    {TEXT_MENU_ADD_BOOK, {"Add book", "Додати книгу"}},
    {TEXT_MENU_SHOW_ALL, {"Show all books", "Показати всі книги"}},
    {TEXT_MENU_EXIT, {"Exit", "Вихід"}},
    {TEXT_PROMPT_TITLE, {"Enter book title: ", "Введіть назву книги: "}},
    {TEXT_PROMPT_AUTHOR, {"Enter book author: ", "Введіть автора книги: "}},
    {TEXT_PROMPT_YEAR, {"Enter publication year: ", "Введіть рік видання: "}},
    {TEXT_LIBRARY_EMPTY, {"The library is empty.", "Бібліотека порожня."}},
    {TEXT_BOOK_ADDED, {"Book added.", "Книгу додано."}},
    {TEXT_BOOK_ADD_FAILED, {"Could not add the book.", "Не вдалося додати книгу."}},
    {TEXT_HEADER_TITLE, {"Title", "Назва"}},
    {TEXT_HEADER_AUTHOR, {"Author", "Автор"}},
    {TEXT_HEADER_YEAR, {"Year", "Рік"}}
};

static const char *localize(Language language, TextKey key)
{
    if (language < 0 || language >= LANGUAGE_COUNT || key < 0 || key >= TEXT_COUNT) {
        return "";
    }

    return localizationDictionary[key].translations[language];
}

#endif
