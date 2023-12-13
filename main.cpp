#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <chrono>
#include <thread>
#include <mutex>


#define PRIME 101
#define MIN_WORDS 4
#define BASE 256

using namespace std;

string readFileAsString(const string& filePath) {
    ifstream file(filePath);
    if (!file.is_open()) 
        throw runtime_error("Error opening file: " + filePath);
    stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    return buffer.str();
}

vector<string> tokenize(const string& str) {
    vector<string> tokens;
    stringstream ss(str);
    string token;

    while (ss >> token) {
        string cleanToken;
        for (char ch : token) {
            if (!ispunct(ch)) 
                cleanToken += ch;
        }
        if (!cleanToken.empty()) 
            tokens.push_back(cleanToken);
    }

    return tokens;
}



string createWordGroup(const vector<string>& words, int start, int size) {
    string group;
    for (int i = start; i < start + size; ++i) {
        group += words[i] + " ";
    }
    group.pop_back(); 
    return group;
}

unsigned long rollingHash(const string& str) {
    unsigned long hash = 0;
    for (char ch : str) {
        hash = (BASE * hash + ch) % PRIME;
    }
    return hash;
}

double calculatePlagiarismPercentage(int plagiarismsCount, int totalWordCount) {
    return static_cast<double>(plagiarismsCount) / totalWordCount * 100;
}

mutex mtx;
void processPart(const vector<string>& words1, const vector<string>& words2,
    const unordered_map<unsigned long, vector<int>>& hashTableText2,
    int start, int end, int words, vector<bool>& plagiarizedIndices,
    vector<int>& sentenceLengths, int& plagiarismsCount) {

    for (int i = start; i <= end - words; ++i) {
        string group1 = createWordGroup(words1, i, words);
        unsigned long hash1 = rollingHash(group1);

        if (hashTableText2.find(hash1) != hashTableText2.end()) {
            for (int j : hashTableText2.at(hash1)) {
                string group2 = createWordGroup(words2, j, words);
                if (group1 == group2) {
                    int extendedWords = words;
                    while ((i + extendedWords < words1.size()) &&
                        (j + extendedWords < words2.size()) &&
                        (words1[i + extendedWords] == words2[j + extendedWords])) {
                        extendedWords++;
                    }

                    if (!plagiarizedIndices[i]) {
                        string extendedGroup = createWordGroup(words1, i, extendedWords);
                        mtx.lock();
                        cout << "~ " << extendedGroup << endl;
                        mtx.unlock();

                        plagiarismsCount++;
                        sentenceLengths[extendedWords]++;

                        for (int k = i; k < i + extendedWords; ++k) {
                            plagiarizedIndices[k] = true;
                        }

                        i += extendedWords - 1; 
                        break;
                    }
                }
            }
        }
    }
}

void findPlagiarism(const string& text1, const string& text2, int words) {
    auto words1 = tokenize(text1);
    auto words2 = tokenize(text2);

    int plagiarismsCount = 0, totalWordCount = words1.size();
    vector<bool> plagiarizedIndices(totalWordCount, false);
    vector<int> sentenceLengths(totalWordCount, 0);

    if (words1.size() < words || words2.size() < words) return;

  
    unordered_map<unsigned long, vector<int>> hashTableText2;
    for (int i = 0; i <= words2.size() - words; ++i) {
        string group = createWordGroup(words2, i, words);
        hashTableText2[rollingHash(group)].push_back(i);
    }

    const int threadCount = 4; 
    vector<thread> threads;
    int partSize = words1.size() / threadCount;

    for (int i = 0; i < threadCount; ++i) {
        int start = i * partSize;
        int end = (i == threadCount - 1) ? words1.size() : start + partSize;
        threads.emplace_back(processPart, ref(words1), ref(words2),
            ref(hashTableText2), start, end, words,
            ref(plagiarizedIndices), ref(sentenceLengths),
            ref(plagiarismsCount));
    }

    for (auto& t : threads) {
        t.join();
    }
    cout << "\n--------------  STATISTICS:  ---------------------------------------------------------- \n\n";
    for (int i = 0; i < sentenceLengths.size(); i++) {
        if (sentenceLengths[i] > 0) {
            cout << "\t" << i << "-words sentences detected: " << sentenceLengths[i] << " times\n";
        }
    }
    int plagiarizedWords = count(plagiarizedIndices.begin(), plagiarizedIndices.end(), true);
    cout << "\n\tTotal words: " << totalWordCount << "\n";
    cout << "\tPlagiarized words: " << plagiarizedWords << "\n";
    cout << "\tPlagiarism score: " << calculatePlagiarismPercentage(plagiarizedWords, totalWordCount) << "%\n\n";
}


int main() {
    try {
        string filePath1 = "c.txt";
        string filePath2 = "d.txt";
        string text1 = readFileAsString(filePath1);
        string text2 = readFileAsString(filePath2);

        cout << "\n--------------------  PLAGIARIZED FRAGMENTS:  ---------------------------------------------------------- \n\n";
        auto startTime = chrono::high_resolution_clock::now();
        findPlagiarism(text1, text2, MIN_WORDS);
        auto endTime = chrono::high_resolution_clock::now();

        chrono::duration<double> executionTime = endTime - startTime;
        cout << "\tExecution time of finding plagiarism and counting statistics: " << executionTime.count() << " seconds\n";
    }
    catch (const exception& e) {
        cerr << "Exception: " << e.what() << endl;
    }

    return 0;
}
