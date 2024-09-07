import sys

# Braille patterns
braille_alphabet = {
    'a': 'O.....', 'b': 'O.O...', 'c': 'OO....', 'd': 'OO.O..', 'e': 'O..O..', 'f': 'OOO...',
    'g': 'OOOO..', 'h': 'O.OO..', 'i': '.OO...', 'j': '.OOO..', 'k': 'O...O.', 'l': 'O.O.O.',
    'm': 'OO..O.', 'n': 'OO.OO.', 'o': 'O..OO.', 'p': 'OOO.O.', 'q': 'OOOOO.', 'r': 'O.OOO.',
    's': '.OO.O.', 't': '.OOOO.', 'u': 'O...OO', 'v': 'O.O.OO', 'w': '.OOO.O', 'x': 'OO..OO',
    'y': 'OO.OOO', 'z': 'O..OOO', ' ': '......',
    'cap': '.....O',  # Capitalization
    'num': '.O.OOO'  # Numbers prefix
}

# Reverse lookup for Braille to English
braille_dict = {v: k for k, v in braille_alphabet.items()}

# Number patterns
numbers = {
    '1': 'O.....', '2': 'O.O...', '3': 'OO....', '4': 'OO.O..', '5': 'O..O..',
    '6': 'OOO...', '7': 'OOOO..', '8': 'O.OO..', '9': '.OO...', '0': '.OOO..'
}


def english_to_braille(text):
    result = []
    is_number = False

    for char in text:
        if char.isupper():
            result.append(braille_alphabet['cap'])
            char = char.lower()

        if char.isdigit():
            if not is_number:
                result.append(braille_alphabet['num'])
                is_number = True
            result.append(numbers[char])
        else:
            is_number = False
            result.append(braille_alphabet.get(char, '......'))  # default for unknown char

    return ''.join(result)


def braille_to_english(braille_text):
    result = []
    is_number = False
    i = 0

    while i < len(braille_text):
        symbol = braille_text[i:i + 6]

        if symbol == braille_alphabet['cap']:
            i += 6
            next_symbol = braille_text[i:i + 6]
            result.append(braille_dict[next_symbol].upper())
        elif symbol == braille_alphabet['num']:
            is_number = True
        else:
            if is_number:
                for num, braille in numbers.items():
                    if braille == symbol:
                        result.append(num)
                        break
            else:
                result.append(braille_dict.get(symbol, '?'))  # Unknown symbol
            is_number = False

        i += 6

    return ''.join(result)


def main():
    if len(sys.argv) < 2:
        print("Please provide a string to translate.")
        return

    input_text = sys.argv[1]

    if input_text[0] in ('O', '.'):
        # Assume Braille input
        translated = braille_to_english(input_text)
    else:
        # Assume English input
        translated = english_to_braille(input_text)

    print(translated)


if __name__ == "__main__":
    main()
