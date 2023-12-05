import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class PlagiarismDetector {

    private static final int PRIME = 101; //hashing int
    private static final int MIN_WORDS = 4; //minimum number of words to be recognized as plagiarism

    public static void main(String[] args) {
        try {
            String text1 = readFileAsString("C:\\Users\\agnie\\IdeaProjects\\PlagiarismDetector\\out\\production\\PlagiarismDetector\\dracula.txt");
            String text2 = readFileAsString("C:\\Users\\agnie\\IdeaProjects\\PlagiarismDetector\\out\\production\\PlagiarismDetector\\frankenstein.txt");

            long startTime = System.currentTimeMillis();
            List<String> plagiarisms = findPlagiarism(text1, text2, MIN_WORDS);
            System.out.println("Plagiarized contents: " + plagiarisms);

            long endTime = System.currentTimeMillis();
            System.out.println("Execution time of finding plagiarism : " + (endTime - startTime) + " milliseconds");

            double plagiarismPercentage = calculatePlagiarismPercentage(plagiarisms, tokenize(text1).length);
            System.out.println("Percentage of plagiarized text: " +  String.format("%.2f", plagiarismPercentage) + "%");


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

    private static long createHash(String str) {
        long hash = 0;
        for (int i = 0; i < str.length(); i++) {
            hash += str.charAt(i) * Math.pow(PRIME, i);
        }
        return hash;
    }

    private static String createWordGroup(String[] words, int start, int size) {
        return String.join(" ", Arrays.copyOfRange(words, start, start + size));
    }

    private static boolean compareStrings(String str1, String str2) {
        return str1.equals(str2);
    }

    public static List<String> findPlagiarism(String text1, String text2, int words) {
        List<String> plagiarisms = new ArrayList<>();

        String[] words1 = tokenize(text1);
        String[] words2 = tokenize(text2);

        long[] hashesText2 = new long[words2.length - words + 1];
        for (int j = 0; j <= words2.length - words; j++) {
            String textGroup = createWordGroup(words2, j, words);
            hashesText2[j] = createHash(textGroup);
        }

        for (int i = 0; i <= words1.length - words; i = i + words) {
            String pattern = createWordGroup(words1, i, words);
            long patternHash = createHash(pattern);

            for (int j = 0; j <= words2.length - words; j++) {
                if (patternHash == hashesText2[j] && compareStrings(pattern, createWordGroup(words2, j, words))) {
                    plagiarisms.add(pattern);
                }
            }
        }
        return plagiarisms;
    }

    private static double calculatePlagiarismPercentage(List<String> plagiarisms, int totalWordCount) {
        int plagiarizedWordCount = plagiarisms.stream()
                .mapToInt(plagiarism -> plagiarism.split(" ").length)
                .sum();

        return (double) plagiarizedWordCount / totalWordCount * 100;
}
}
