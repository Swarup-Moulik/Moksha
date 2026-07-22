# Fibonacci
hyperfine -N --warmup 3 --runs 10 --ignore-failure './fibonacci_c.exe' './fibonacci_mox.exe' --export-json fib_results.json

# Matmul (1024x1024)
hyperfine -N --warmup 3 --runs 5 --ignore-failure './matmul_c.exe' './matmul_mox.exe' --export-json matmul_results.json

# Sieve (1M)
hyperfine -N --warmup 3 --runs 5 --ignore-failure './sieve_c.exe' './sieve_mox.exe' --export-json sieve_results.json
