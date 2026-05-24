import os
import dotenv

def main() -> None:
    dotenv.load_dotenv()

    for k, v in os.environ.items():
        print(f"{k} -> {v}")

if __name__ == "__main__":
    main()