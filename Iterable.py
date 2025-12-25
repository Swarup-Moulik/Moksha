# =========================
# BENCHMARK CONFIG
# =========================
ITERATIONS = 100_000


# =========================
# 1. INPUT (ONCE)
# =========================
print("\nEnter a string: ", end="")
w = input()

print("Enter array size: ", end="")
size = int(input())

ar_input = []

print(f"Enter {size} words for the array:")
for i in range(size):
    print(f"ar[{i}]: ", end="")
    ar_input.append(input())

print("\nEnter a table key: ", end="")
k = input()

print(f"Enter value for {k}: ", end="")
k_value = input()


# =========================
# 2. BENCHMARK LOOP
# =========================
for _ in range(ITERATIONS):

    # --- fresh copies per iteration ---
    ar = list(ar_input)
    tab = {}

    tab[k] = k_value
    tab["fixed"] = "unchangeable"

    # --- Array loops ---
    for val in ar:
        pass

    for idx, val in enumerate(ar):
        pass

    for val in ar:
        pass

    # --- Table loops ---
    for key, val in tab.items():
        pass

    for key in tab:
        pass

    # --- String iteration ---
    if len(w) > 0:
        for ch in w:
            pass

    # --- Complex assignment ---
    if len(w) > 3:
        ch = w[3]
        if len(ar) > 3:
            ar[3] = ch

    if len(ar) > 3:
        tab["fromArray"] = ar[3]

    _ = len(ar)

    # --- Deletion (COMMENTED OUT) ---
    """
    if len(ar) > 3:
        del ar[3]

    if k in tab:
        del tab[k]
    """


# =========================
# 3. FINAL OUTPUT
# =========================
print("\n=== TEST COMPLETED ===")
print(f"Iterations executed: {ITERATIONS}")
