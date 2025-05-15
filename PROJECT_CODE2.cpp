#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <random>
#include <deque>
#include <iomanip>

struct Song {
    std::string title;
    std::string artist;
    std::string album;
    int duration;
    bool favorite;
    std::string filePath;

    Song() : title(""), artist(""), album(""), duration(0), favorite(false), filePath("") {}
    Song(const std::string& t, const std::string& ar, const std::string& al, int dur, bool fav = false, const std::string& fp = "")
        : title(t), artist(ar), album(al), duration(dur), favorite(fav), filePath(fp) {}
};

struct Node {
    Song data;
    Node* next;
    Node* prev;
    Node(const Song& s) : data(s), next(nullptr), prev(nullptr) {}
};

class Playlist {
private:
    Node* head;
    Node* tail;
    Node* current;

    enum class PlaybackState { Stopped, Playing, Paused };
    PlaybackState state;

    std::deque<Song> history;
    size_t maxHistorySize = 10;

    std::vector<Node*> shuffledOrder;
    size_t shuffleIndex;
    bool isShuffled;
    bool repeat;

    void clearNodes() {
        Node* temp = head;
        while (temp) {
            Node* nextNode = temp->next;
            delete temp;
            temp = nextNode;
        }
        head = tail = current = nullptr;
        shuffledOrder.clear();
        history.clear();
        isShuffled = false;
    }

    std::vector<Node*> getNodeVector() const {
        std::vector<Node*> nodes;
        Node* temp = head;
        while (temp) {
            nodes.push_back(temp);
            temp = temp->next;
        }
        return nodes;
    }

    void relinkNodes(const std::vector<Node*>& nodes) {
        for (size_t i = 0; i < nodes.size(); ++i) {
            nodes[i]->prev = (i > 0) ? nodes[i - 1] : nullptr;
            nodes[i]->next = (i + 1 < nodes.size()) ? nodes[i + 1] : nullptr;
        }
        head = nodes.front();
        tail = nodes.back();
        current = head;
    }

    void addToHistory(const Song& song) {
        if (history.size() >= maxHistorySize)
            history.pop_front();
        history.push_back(song);
    }
    void saveToFile(const std::string& filename) const {
    std::ofstream out(filename);
    if (!out) {
        std::cerr << "Error: Unable to open file for writing.\n";
        return;
    }

    Node* temp = head;
    while (temp) {
        out << temp->data.title << "," 
            << temp->data.artist << "," 
            << temp->data.album << "," 
            << temp->data.duration << "," 
            << temp->data.favorite << "," 
            << temp->data.filePath << "\n";
        temp = temp->next;
    }

    out.close();
    std::cout << "Playlist saved to " << filename << "\n";
}

public:
    std::string name;

    explicit Playlist(const std::string& n = "New Playlist")
        : head(nullptr), tail(nullptr), current(nullptr),
          state(PlaybackState::Stopped), shuffleIndex(0), isShuffled(false), repeat(false), name(n) {}

    ~Playlist() { clearNodes(); }

    void addSong(const Song& song) {
        Node* newNode = new Node(song);
        if (!head) {
            head = tail = newNode;
        } else {
            tail->next = newNode;
            newNode->prev = tail;
            tail = newNode;
        }
        if (!current)
            current = head;
    }

    void removeSong(const std::string& title) {
        Node* temp = head;
        while (temp && temp->data.title != title)
            temp = temp->next;
        if (temp) {
            if (temp->prev) temp->prev->next = temp->next;
            else head = temp->next;
            if (temp->next) temp->next->prev = temp->prev;
            else tail = temp->prev;

            auto it = std::find(shuffledOrder.begin(), shuffledOrder.end(), temp);
            if (it != shuffledOrder.end()) shuffledOrder.erase(it);
            if (current == temp) current = temp->next ? temp->next : temp->prev;
            delete temp;
        }
    }

    void modifySong(const std::string& title, const Song& newSong) {
        Node* temp = head;
        while (temp && temp->data.title != title)
            temp = temp->next;
        if (temp) temp->data = newSong;
    }

    void searchSong(const std::string& query) const {
        Node* temp = head;
        while (temp) {
            if (temp->data.title.find(query) != std::string::npos ||
                temp->data.artist.find(query) != std::string::npos) {
                std::cout << "Found: " << temp->data.title << " by " << temp->data.artist << "\n";
            }
            temp = temp->next;
        }
    }

    void sortByTitle() {
        std::vector<Node*> nodes = getNodeVector();
        std::sort(nodes.begin(), nodes.end(), [](Node* a, Node* b) {
            return a->data.title < b->data.title;
        });
        relinkNodes(nodes);
    }

    void displayAll() const {
        Node* temp = head;
        while (temp) {
            std::cout << (temp == current ? "--> " : "    ");
            std::cout << temp->data.title << " | " << temp->data.artist << " | "
                      << temp->data.album << " | " << temp->data.duration << "s";
            if (temp->data.favorite) std::cout << " [Favorite]";
            std::cout << "\n";
            temp = temp->next;
        }
    }

    void toggleFavorite(const std::string& title) {
        Node* temp = head;
        while (temp) {
            if (temp->data.title == title) {
                temp->data.favorite = !temp->data.favorite;
                break;
            }
            temp = temp->next;
        }
    }

    int getTotalDuration() const {
        int total = 0;
        Node* temp = head;
        while (temp) {
            total += temp->data.duration;
            temp = temp->next;
        }
        return total;
    }

    void toggleRepeat(bool enabled) {
        repeat = enabled;
    }

    void play() {
        if (!current) return;
        state = PlaybackState::Playing;
        addToHistory(current->data);
    
        std::cout << "Playing: " << current->data.title << " by " << current->data.artist << "\n";
    
        std::string command = "start \"\" \"" + current->data.filePath + "\""; // for Windows
        system(command.c_str());
    }

    void next() {
        if (!current) return;
        if (isShuffled && shuffleIndex + 1 < shuffledOrder.size())
            current = shuffledOrder[++shuffleIndex];
        else if (!isShuffled && current->next)
            current = current->next;
        else if (repeat)
            current = head;
        play();
    }

    void prev() {
        if (!current) return;
        if (isShuffled && shuffleIndex > 0)
            current = shuffledOrder[--shuffleIndex];
        else if (!isShuffled && current->prev)
            current = current->prev;
        else if (repeat)
            current = tail;
        play();
    }

    void toggleShuffle(bool enable) {
        isShuffled = enable;
        if (enable) {
            shuffledOrder = getNodeVector();
            std::shuffle(shuffledOrder.begin(), shuffledOrder.end(), std::mt19937(std::random_device{}()));
            shuffleIndex = 0;
            current = shuffledOrder[0];
        } else {
            current = head;
        }
    }
};

int main() {
    Playlist pl("My Playlist");

    int choice;
    do {
        std::cout << "\n--- Playlist Menu ---\n";
        std::cout << "1. Add Song\n";
        std::cout << "2. Remove Song\n";
        std::cout << "3. Modify Song\n";
        std::cout << "4. Display Songs\n";
        std::cout << "5. Search Song\n";
        std::cout << "6. Sort Songs by Title\n";
        std::cout << "7. Toggle Favorite\n";
        std::cout << "8. Show Total Duration\n";
        std::cout << "9. Play\n";
        std::cout << "10. Next\n";
        std::cout << "11. Previous\n";
        std::cout << "12. Toggle Repeat\n";
        std::cout << "13. Toggle Shuffle\n";
        std::cout << "0. Exit\n";
        std::cout << "Enter choice: ";
        std::cin >> choice;

        std::cin.ignore();
        std::string title, artist, album, path;
int duration;

switch (choice) {
    case 1: {
        std::cout << "Title: "; getline(std::cin, title);
        std::cout << "Artist: "; getline(std::cin, artist);
        std::cout << "Album: "; getline(std::cin, album);
        std::cout << "Duration (sec): "; std::cin >> duration;
        std::cin.ignore(); // to flush newline
        std::cout << "File Path (e.g. C:/Music/song.mp3): "; getline(std::cin, path);
        pl.addSong(Song(title, artist, album, duration, false, path));
        break;
    }  
        case 2:
            std::cout << "Title to remove: "; getline(std::cin, title);
            pl.removeSong(title);
            break;
        case 3:
            std::cout << "Title to modify: "; getline(std::cin, title);
            std::cout << "New Title: "; getline(std::cin, artist);
            std::cout << "New Artist: "; getline(std::cin, album);
            std::cout << "New Album: "; getline(std::cin, title);
            std::cout << "New Duration (sec): "; std::cin >> duration;
            pl.modifySong(title, Song(artist, album, title, duration));
            break;
        case 4:
            pl.displayAll();
            break;
        case 5:
            std::cout << "Search query: "; getline(std::cin, title);
            pl.searchSong(title);
            break;
        case 6:
            pl.sortByTitle();
            break;
        case 7:
            std::cout << "Title to toggle favorite: "; getline(std::cin, title);
            pl.toggleFavorite(title);
            break;
        case 8:
            std::cout << "Total Duration: " << pl.getTotalDuration() << " seconds\n";
            break;
        case 9:
            pl.play();
            break;
        case 10:
            pl.next();
            break;
        case 11:
            pl.prev();
            break;
        case 12:
            pl.toggleRepeat(true);
            break;
        case 13:
            pl.toggleShuffle(true);
            break;
        case 0:
            std::cout << "Exiting...\n";
            break;
        default:
            std::cout << "Invalid choice.\n";
        }
        std::cin.ignore();
    } while (choice != 0);

    return 0;
}
