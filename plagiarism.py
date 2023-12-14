from threading import Thread, Lock
import time

PRIME = 101
MIN_WORDS = 4
BASE = 256

def readFileAsString(file_path):
    with open(file_path, 'r', encoding='utf-8') as file:
        return file.read()

def tokenize(text):
    tokens = []
    for token in text.split():
        clean_token = ''.join(ch for ch in token if ch.isalnum())
        if clean_token:
            tokens.append(clean_token)
    return tokens

def createWordGroup(words, start, size):
    group = ' '.join(words[start:start + size])
    return group

def rollingHash(text):
    hash_val = 0
    for ch in text:
        hash_val = (BASE * hash_val + ord(ch)) % PRIME
    return hash_val

def calculatePlagiarismPercentage(plagiarisms_count, total_word_count):
    return (plagiarisms_count / total_word_count) * 100 if total_word_count > 0 else 0

def processPart(words1, words2, hash_table_text2, start, end, words, plagiarized_indices, sentence_lengths, plagiarisms_count, print_lock):
    for i in range(start, end - words):
        group1 = createWordGroup(words1, i, words)
        hash1 = rollingHash(group1)

        if hash1 in hash_table_text2:
            for j in hash_table_text2[hash1]:
                group2 = createWordGroup(words2, j, words)
                if group1 == group2:
                    extended_words = words
                    while (i + extended_words < len(words1)) and (j + extended_words < len(words2)) and (words1[i + extended_words] == words2[j + extended_words]):
                        extended_words += 1

                    if not plagiarized_indices[i]:
                        extended_group = createWordGroup(words1, i, extended_words)
                        with print_lock:
                            print("~", extended_group)

                        plagiarisms_count += 1
                        sentence_lengths[extended_words] += 1

                        for k in range(i, i + extended_words):
                            plagiarized_indices[k] = True

                        i += extended_words - 1
                        break

def findPlagiarism(text1, text2, words):
    words1 = tokenize(text1)
    words2 = tokenize(text2)

    plagiarisms_count = 0
    total_word_count = len(words1)
    plagiarized_indices = [False] * total_word_count
    sentence_lengths = [0] * total_word_count

    if len(words1) < words or len(words2) < words:
        return

    hash_table_text2 = {}
    for i in range(len(words2) - words + 1):
        group = createWordGroup(words2, i, words)
        hash_table_text2.setdefault(rollingHash(group), []).append(i)

    thread_count = 4
    threads = []
    print_lock = Lock()
    part_size = total_word_count // thread_count

    for i in range(thread_count):
        start = i * part_size
        end = min(start + part_size, total_word_count)
        threads.append(Thread(target=processPart, args=(words1, words2, hash_table_text2, start, end, words, plagiarized_indices, sentence_lengths, plagiarisms_count, print_lock)))

    for thread in threads:
        thread.start()

    for thread in threads:
        thread.join()

    print("\n--------------  STATISTICS:  ---------------------------------------------------------- \n\n")
    for i, count in enumerate(sentence_lengths):
        if count > 0:
            print(f"\t{i}-words sentences detected: {count} times")

    plagiarized_words = sum(plagiarized_indices)
    print(f"\n\tTotal words: {total_word_count}")
    print(f"\tPlagiarized words: {plagiarized_words}")
    print(f"\tPlagiarism score: {calculatePlagiarismPercentage(plagiarized_words, total_word_count)}%\n")

if __name__ == "__main__":
    try:
        file_path1 = "frankenstein.txt"
        file_path2 = "dracula.txt"
        text1 = readFileAsString(file_path1)
        text2 = readFileAsString(file_path2)

        print("\n--------------------  PLAGIARIZED FRAGMENTS:  ---------------------------------------------------------- \n\n")
        start_time = time.time()
        findPlagiarism(text1, text2, MIN_WORDS)
        end_time = time.time()

        execution_time = end_time - start_time
        print(f"\tExecution time of finding plagiarism and counting statistics: {execution_time} seconds\n")
    except Exception as e:
        print(f"An error occurred: {e}")
