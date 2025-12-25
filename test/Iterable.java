import java.util.*;

public class Iterable {
    public static void main(String[] args) {
        Scanner sc = new Scanner(System.in);

        // 1. INPUT
        System.out.print("\nEnter a string: ");
        String w = sc.nextLine();

        System.out.print("Enter array size: ");
        int size = Integer.parseInt(sc.nextLine());

        List<String> ar = new ArrayList<>(size);

        System.out.println("Enter " + size + " words for the array:");
        for (int i = 0; i < size; i++) {
            System.out.print("ar[" + i + "]: ");
            ar.add(sc.nextLine());
        }

        Map<String, String> tab = new HashMap<>();

        System.out.print("\nEnter a table key: ");
        String k = sc.nextLine();

        System.out.print("Enter value for " + k + ": ");
        tab.put(k, sc.nextLine());

        tab.put("fixed", "unchangeable");

        // 2. DISPLAY LOOPS
        System.out.println("\n--- Loop Display ---");

        System.out.println("Pretty Print Array :- " + ar);

        System.out.println("\nArray (For-In Value Only):");
        for (String val : ar) {
            System.out.println(val);
        }

        System.out.println("\nArray (For-In Index & Value):");
        for (int idx = 0; idx < ar.size(); idx++) {
            System.out.println("[" + idx + "] = " + ar.get(idx));
        }

        System.out.println("\nArray (For-In Value):");
        for (String val : ar) {
            System.out.println("Element = " + val);
        }

        System.out.println("\nTable (Key, Value):");
        for (Map.Entry<String, String> entry : tab.entrySet()) {
            System.out.println("Key: " + entry.getKey() + ", Val: " + entry.getValue());
        }

        System.out.println("\nTable (Key):");
        for (String key : tab.keySet()) {
            System.out.println("Key: " + key);
        }

        // String Iteration
        System.out.println("\nString (For-In):");
        if (w.length() > 0) {
            for (int i = 0; i < w.length(); i++) {
                System.out.println(w.charAt(i));
            }
        }

        // 3. COMPLEX ASSIGNMENT
        System.out.println("\n--- Complex Assignment ---");

        if (w.length() > 3) {
            char ch = w.charAt(3);
            System.out.println("4th letter of string: " + ch);

            if (ar.size() > 3) {
                ar.set(3, String.valueOf(ch));
                System.out.println("Assigned ar[3] = " + ch);
            }
        } else {
            System.out.println("String too short for index 3 test.");
        }

        if (ar.size() > 3) {
            String valAr = ar.get(3);
            tab.put("fromArray", valAr);
            System.out.println("Assigned tab['fromArray'] = " + valAr);
        }

        System.out.println("Table : " + tab);
        System.out.println("Length of array : " + ar.size());

        // 4. DELETION
        System.out.println("\n--- Deletion ---");

        if (ar.size() > 3) {
            System.out.println("Deleting ar[3] (" + ar.get(3) + ")...");
            ar.remove(3);
            System.out.println("Array after delete: " + ar);
        }

        System.out.println("Length of array : " + ar.size());

        System.out.println("Deleting tab[" + k + "]...");
        tab.remove(k);
        System.out.println("Table after delete: " + tab);

        System.out.println("\n=== TEST COMPLETED ===");
        sc.close();
    }
}
