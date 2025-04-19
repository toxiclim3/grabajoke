#define CPPHTTPLIB_OPENSSL_SUPPORT // vcpkg install openssl:x64-windows
#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <conio.h>
#include "httplib.h"
#include "json.hpp"
#include <windows.h>

using json = nlohmann::json;
using namespace std;

struct JokeModel {
    string joke;
    string setup;
    string delivery;
    string type;

    string getContent() const {
        return (type == "single") ? joke : (setup + "\n" + delivery);
    }
};

void from_json(const json& j, JokeModel& joke) {
    joke = JokeModel{};
    if (!j.is_object()) return;
    if (j.contains("error") && j["error"].get<bool>()) return;
    if (j.contains("type") && j["type"].is_string()) j["type"].get_to(joke.type);
    if (joke.type == "single" && j.contains("joke")) j["joke"].get_to(joke.joke);
    else if (joke.type == "twopart") {
        if (j.contains("setup")) j["setup"].get_to(joke.setup);
        if (j.contains("delivery")) j["delivery"].get_to(joke.delivery);
    }
}

class JokePrinter {
    static const int BoxWidth = 52;

public:
    static void printJoke(const JokeModel& joke, int index) {
        string jokeContent = joke.getContent();
        vector<string> wrappedContent = wrapText(jokeContent, BoxWidth - 4);
        int boxHeight = wrappedContent.size() + 4;

        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

        for (int y = 0; y < boxHeight; y++) {
            cout << string(BoxWidth, ' ') << "\n";
        }

        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(hConsole, &csbi);
        COORD pos = { csbi.dwCursorPosition.X, csbi.dwCursorPosition.Y - boxHeight };
        SetConsoleCursorPosition(hConsole, pos);

        pair<WORD, WORD> colors = getFrameAndTextColors();
        WORD frameColor = colors.first;
        WORD textColor = colors.second;

        SetConsoleTextAttribute(hConsole, frameColor);

        if (index != 0)
        {
            string header = " Жарт №" + to_string(index) + " ";
            cout << string((BoxWidth - header.length()) / 2, ' ') << header << "\n";
        }
        

        cout << "#" << string(BoxWidth - 2, '#') << "#\n";

        SetConsoleTextAttribute(hConsole, textColor);

        for (const string& line : wrappedContent) {
            string paddedLine = line.length() > BoxWidth - 4 ? line.substr(0, BoxWidth - 4) : line + string(BoxWidth - 4 - line.length(), ' ');
            cout << "# " << paddedLine << " #\n";
        }

        SetConsoleTextAttribute(hConsole, frameColor);
        cout << "#" << string(BoxWidth - 2, '#') << "#\n";

        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        cout << "\n";
    }

private:
    static vector<string> wrapText(const string& text, int lineWidth) {
        vector<string> lines;
        string remaining = text;

        size_t pos = 0;
        while ((pos = remaining.find('\n')) != string::npos) {
            string part = remaining.substr(0, pos);
            while (part.length() > static_cast<size_t>(lineWidth)) {
                size_t splitIndex = part.rfind(' ', lineWidth);
                if (splitIndex == string::npos) splitIndex = lineWidth;
                lines.push_back(part.substr(0, splitIndex));
                part = part.substr(splitIndex);
                while (!part.empty() && part[0] == ' ') part = part.substr(1);
            }
            lines.push_back(part);
            remaining = remaining.substr(pos + 1);
        }

        while (remaining.length() > static_cast<size_t>(lineWidth)) {
            size_t splitIndex = remaining.rfind(' ', lineWidth);
            if (splitIndex == string::npos) splitIndex = lineWidth;
            lines.push_back(remaining.substr(0, splitIndex));
            remaining = remaining.substr(splitIndex);
            while (!remaining.empty() && remaining[0] == ' ') remaining = remaining.substr(1);
        }
        if (!remaining.empty()) lines.push_back(remaining);

        return lines;
    }

    static pair<WORD, WORD> getFrameAndTextColors() {
        srand((time(0)));

        int color = rand() % 6 + 1;
        pair<WORD, WORD> result;
        result.first = color;
        result.second = color + 8;
        return result;
    }
};

class JokeApi {
    httplib::Client client;

public:
    JokeApi(const string& baseUrl) : client(baseUrl) {}

    JokeModel getJoke(const string& type,std::string& kind) {
        string path;
        if (strcmp(kind.c_str(), "Dark") == 0)
        {
            path = "/joke/" + kind + (type.empty() ? "" : "?type=" + type);
        }
        else
        {
            path = "/joke/" + kind + (type.empty() ? "?safe-mode" : "?safe-mode&type=" + type);
        }
        
        auto res = client.Get(path.c_str());
        if (!res || res->status != 200) return JokeModel{};
        try {
            json j = json::parse(res->body);
            JokeModel joke;
            from_json(j, joke);
            return joke;
        }
        catch (const exception&) {
            return JokeModel{};
        }
    }

    vector<JokeModel> fetchJokes(int totalJokes,std::string kind) {
        vector<JokeModel> jokes;
        int jokeCount = 0;

        if (totalJokes != 1)
        {
            cout << "Завантаження жартiв...\n";
        }
        else
        {
            cout << "Завантаження жарта...\n";
        }
        

        while (jokeCount < totalJokes) {
            JokeModel joke = getJoke("",kind);
            if (!joke.type.empty()) {
                jokes.push_back(joke);

                if (totalJokes == 1)
                {
                    JokePrinter::printJoke(joke, 0);
                }
                else 
                {
                    JokePrinter::printJoke(joke, jokeCount + 1);
                }
                
                jokeCount++;
            }
            else {
                cout << "Помилка, пропускаємо цей жарт...\n";
            }
        }

        cout << "Готово!\n";
        if (jokes.empty()) {
            cout << "Щось пішло не так...\n";
        }
        return jokes;
    }
};

void menu(JokeApi& jokeApi)
{
    std::cout << "Welcome!\nPress ENTER to get a random joke\nPress 1 to get a programming joke\nPress 2 to get a dark joke\nPress 3 to get a pun\nPress 4 to get a spooky joke\nPress 5 to get a christmas joke\nPress ESCAPE to exit\n";
    bool exit = 0;

    while (!exit)
    {
        if (_kbhit()) // если было нажатие на клавиши пользователем  
        {
            int code = _getch();

            switch (code)
            {
                case 13:
                {
                    jokeApi.fetchJokes(1, string("Any"));
                    break;
                }
                case 27:
                {
                    exit = 1;
                    break;
                }
                case 49:
                {
                    jokeApi.fetchJokes(1, string("Programming"));
                    break;
                }
                case 50:
                {
                    jokeApi.fetchJokes(1, string("Dark"));
                    break;
                }
                case 51:
                {
                    jokeApi.fetchJokes(1, string("Pun"));
                    break;
                }
                case 52:
                {
                    jokeApi.fetchJokes(1, string("Spooky"));
                    break;
                }
                case 53:
                {
                    jokeApi.fetchJokes(1, string("Christmas"));
                    break;
                }
            }
        }
    }
}

int main() {
    setlocale(0, "");
    system("title Joke API");

    JokeApi jokeApi("https://v2.jokeapi.dev");


    menu(jokeApi);
}