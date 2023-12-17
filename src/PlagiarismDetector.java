import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.atomic.AtomicInteger;

public class PlagiarismDetector {

    private static final int PRIME = 101; //hashing int
    private static final int BASE = 256;
    private static final int MIN_WORDS = 4; //minimum number of words to be recognized as plagiarism

    public static void main(String[] args) {
        try {
            String text1 = readFileAsString("frankenstein.txt");
            String text2 = readFileAsString("dracula.txt");

            long startTime = System.currentTimeMillis();
            findPlagiarism(text1, text2, MIN_WORDS);
            long endTime = System.currentTimeMillis();
            System.out.printf("Execution time of finding plagiarism  %.2f seconds%n", (endTime - startTime) / 1000.0);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private static String readFileAsString(String filePath) throws IOException {
        return new String(Files.readAllBytes(Paths.get(filePath)));
    }

    private static String[] tokenize(String str) {
        return str.replaceAll("[^a-zA-Z ]", "").toLowerCase().split("\\s+");
    }

    private static int rollingHash(String text) {
        int hash = 0;
        for (char ch : text.toCharArray()) {
            hash = (BASE * hash + ch) % PRIME;
        }
        return hash;
    }

    private static String createWordGroup(String[] words, int start, int size) {
        return String.join(" ", Arrays.copyOfRange(words, start, start + size));
    }

    public static void findPlagiarism(String text1, String text2, int words) {
        String[] words1 = tokenize(text1);
        String[] words2 = tokenize(text2);

        AtomicInteger plagiarismsCount = new AtomicInteger();
        ConcurrentHashMap<Integer, Boolean> plagiarizedIndices = new ConcurrentHashMap<>();
        ConcurrentHashMap<Integer, AtomicInteger> sentenceLengths = new ConcurrentHashMap<>();

        ConcurrentHashMap<Integer, List<Integer>> hashTableText2 = new ConcurrentHashMap<>();
        for (int i = 0; i <= words2.length - words; i++) {
            String group = createWordGroup(words2, i, words);
            int hash = rollingHash(group);
            hashTableText2.computeIfAbsent(hash, k -> new ArrayList<>()).add(i);
        }

        int totalWordCount = words1.length;
        int partSize = totalWordCount / 4;

        try (ExecutorService executor = Executors.newFixedThreadPool(4)) {
            for (int i = 0; i < 4; i++) {
                int start = i * partSize;
                int end = Math.min(start + partSize, totalWordCount);
                executor.execute(() -> processPart(words1, words2, hashTableText2, start, end, words, plagiarizedIndices, sentenceLengths, plagiarismsCount));
            }
        } catch (Exception e) {
            e.printStackTrace();
        }

        printStatistics(sentenceLengths, totalWordCount, plagiarizedIndices);
    }

    private static void processPart(String[] words1, String[] words2, ConcurrentHashMap<Integer, List<Integer>> hashTableText2,
                                    int start, int end, int words, ConcurrentHashMap<Integer, Boolean> plagiarizedIndices,
                                    ConcurrentHashMap<Integer, AtomicInteger> sentenceLengths, AtomicInteger plagiarismsCount) {
        for (int i = start; i < end - words; i++) {
            String group1 = createWordGroup(words1, i, words);
            int hash1 = rollingHash(group1);

            if (hashTableText2.containsKey(hash1)) {
                for (Integer j : hashTableText2.get(hash1)) {
                    String group2 = createWordGroup(words2, j, words);
                    if (group1.equals(group2)) {
                        int extendedWords = words;
                        while (i + extendedWords < words1.length && j + extendedWords < words2.length
                                && words1[i + extendedWords].equals(words2[j + extendedWords])) {
                            extendedWords++;
                        }

                        if (plagiarizedIndices.putIfAbsent(i, true) == null) {
                            String extendedGroup = createWordGroup(words1, i, extendedWords);
                            System.out.println("~ " + extendedGroup);

                            plagiarismsCount.incrementAndGet();
                            sentenceLengths.computeIfAbsent(extendedWords, k -> new AtomicInteger()).incrementAndGet();

                            for (int k = i; k < i + extendedWords; k++) {
                                plagiarizedIndices.put(k, true);
                            }

                            i += extendedWords - 1;
                            break;
                        }
                    }
                }
            }
        }
    }

    private static void printStatistics(ConcurrentHashMap<Integer, AtomicInteger> sentenceLengths, int totalWordCount,
                                        ConcurrentHashMap<Integer, Boolean> plagiarizedIndices) {
        System.out.println("\n--------------  STATISTICS:  ---------------------------------------------------------- \n\n");
        sentenceLengths.forEach((length, count) -> {
            if (count.get() > 0) {
                System.out.printf("\t%d-words sentences detected: %d times%n", length, count.get());
            }
        });

        long plagiarizedWords = plagiarizedIndices.values().stream().filter(b -> b).count();
        double plagiarismPercentage = ((double) plagiarizedWords / totalWordCount) * 100;
        System.out.printf("\n\tTotal words: %d%n", totalWordCount);
        System.out.printf("\tPlagiarized words: %d%n", plagiarizedWords);
        System.out.printf("\tPlagiarism score: %.2f%%%n", plagiarismPercentage);
    }
}
