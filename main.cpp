#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <chrono>
#include <thread>
#include <mutex>
#include <regex>

#define PRIME 101
#define MIN_WORDS 4
#define BASE 256

using namespace std;

string readfile(const string& path) {
    ifstream file(path);
    if (!file.is_open())
        throw runtime_error("Error opening file: " + path);
    stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    return buffer.str();
}


vector<string> tokenize(const string& str) {
    vector<string> tokens;
    string temp;

    const string delimiters = " \n\t\r,;:!?'\"()[]{}_*#—-’”“™";
    const string specialDelimiters = "."; 

    for (char ch : str) {
        if (delimiters.find(ch) != string::npos) {
            if (!temp.empty()) {
                tokens.push_back(temp);
                temp.clear();
            }
        }
        else if (specialDelimiters.find(ch) != string::npos) {
            if (!temp.empty()) {
                tokens.push_back(temp);
                temp.clear();
            }
        }
        else {
            temp += ch;
        }
    }

    if (!temp.empty()) {
        tokens.push_back(temp);
    }

    return tokens;
}


string creategroup(const vector<string>& words, int str, int size) {
    string group;
    for (int i = str; i < str + size; ++i)
        group += words[i] + " ";
    group.pop_back();
    return group;
}

unsigned long rhash(const string& str) {
    unsigned long hash = 0;
    for (char ch : str)
        hash = (BASE * hash + ch) % PRIME;
    return hash;
}

double calculateplagiarismpercentage(int Count, int WordCount) {
    return static_cast<double>(Count) / WordCount * 100;
}

mutex mtx;
void processp(const vector<string>& word1, const vector<string>& word2,
    const unordered_map<unsigned long, vector<int>>& hashtabletext,
    int start, int end, int words, vector<bool>& pindices,
    vector<int>& slengths, int& pcount) {

    for (int i = start; i <= end - words; ++i) {
        string group1 = creategroup(word1, i, words);
        unsigned long hash1 = rhash(group1);

        if (hashtabletext.find(hash1) != hashtabletext.end()) {
            for (int j : hashtabletext.at(hash1)) {
                string group2 = creategroup(word2, j, words);
                if (group1 == group2) {
                    int extendedwords = words;
                    while ((i + extendedwords < word1.size()) &&
                        (j + extendedwords < word2.size()) &&
                        (word1[i + extendedwords] == word2[j + extendedwords])) {
                        extendedwords++;
                    }
                    if (!pindices[i]) {
                        string extendedgroup = creategroup(word1, i, extendedwords);
                        mtx.lock();
                        cout << "~ " << extendedgroup << endl;
                        mtx.unlock();
                        pcount++;
                        slengths[extendedwords]++;
                        for (int k = i; k < i + extendedwords; ++k)
                            pindices[k] = true;
                        i += extendedwords - 1;
                        break;
                    }
                }
            }
        }
    }
}

void findplagiarism(const string& text1, const string& text2, int words) {
    auto words1 = tokenize(text1);
    auto words2 = tokenize(text2);

    int pcount = 0, wordcount = words1.size();
    vector<bool> pindices(wordcount, false);
    vector<int> slengths(wordcount, 0);

    if (words1.size() < words || words2.size() < words) return;
    unordered_map<unsigned long, vector<int>> hashTableText2;
    for (int i = 0; i <= words2.size() - words; ++i) {
        string group = creategroup(words2, i, words);
        hashTableText2[rhash(group)].push_back(i);
    }

    const int threadcount = 4;
    vector<thread> threads;
    int psize = words1.size() / threadcount;
    for (int i = 0; i < threadcount; ++i) {
        int start = i * psize;
        int end = (i == threadcount - 1) ? words1.size() : start + psize;
        threads.emplace_back(processp, ref(words1), ref(words2),
            ref(hashTableText2), start, end, words,
            ref(pindices), ref(slengths),
            ref(pcount));
    }
    for (auto& t : threads)
        t.join();

    cout << "\n--------------  STATISTICS:  ---------------------------------------------------------- \n\n";
    for (int i = 0; i < slengths.size(); i++)
        if (slengths[i] > 0) {
            cout << "\t" << i << "-words sentences detected: " << slengths[i] << " times\n";
        }
    int plagiarizedwords = count(pindices.begin(), pindices.end(), true);
    cout << "\n\tTotal words: " << wordcount << "\n";
    cout << "\tPlagiarized words: " << plagiarizedwords << "\n";
    cout << "\tPlagiarism score: " << calculateplagiarismpercentage(plagiarizedwords, wordcount) << "%\n\n";
}

int main() {
    try {
        string Path1 = "a.txt";
        string Path2 = "b.txt";
        string text1 = readfile(Path1);
        string text2 = readfile(Path2);
        cout << "\n--------------------  PLAGIARIZED FRAGMENTS:  ---------------------------------------------------------- \n\n";
        auto start = chrono::high_resolution_clock::now();
        findplagiarism(text1, text2, MIN_WORDS);
        auto stop = chrono::high_resolution_clock::now();
        chrono::duration<double> executionTime = stop - start;
        cout << "\tExecution time of finding plagiarism and counting statistics: " << executionTime.count() << " seconds\n";
    }
    catch (const exception& e) {
        cerr << "Exception: " << e.what() << endl;
    }
    return 0;
}