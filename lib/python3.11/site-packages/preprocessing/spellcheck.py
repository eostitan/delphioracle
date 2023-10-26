'''
Spelling checker module:
'''


import re
from os import path
from collections import Counter

from preprocessing.errors import InputError


EN_ALPHABET = 'abcdefghijklmnopqrstuvwxyz'
WORD_DISTRIBUTION = Counter(re.findall(r'\w+', open(path.join(path.dirname(__file__), 'data/bnc_wiktionary_corpus.txt')).read().lower()))


#functions
def correct_word(word_string):
    '''
    Finds all valid one and two letter corrections for word_string, returning the word
    with the highest relative probability as type str.
    '''
    if word_string is None:
        return ""
    elif isinstance(word_string, str):
        return max(find_candidates(word_string), key=find_word_prob)
    else:
        raise InputError("string or none type variable not passed as argument to correct_word")

def find_candidates(word_string):
    '''
    Finds all potential words word_string could have intended to mean. If a word is not incorrectly
    spelled, it will return this word first, else if will look for one letter edits that are correct.
    If there are no valid one letter edits, it will perform a two letter edit search.

    If valid corrections are found, all are returned as a set instance. Should a valid word not be
    found, the original word is returned as a set instance.
    '''
    if word_string is None:
        return {}
    elif isinstance(word_string, str):
        return (validate_words([word_string]) or validate_words(list(find_one_letter_edits(word_string)))
                or validate_words(list(find_two_letter_edits(word_string))) or set([word_string]))
    else:
        raise InputError("string or none type variable not passed as argument to find_candidates")

def find_one_letter_edits(word_string):
    '''
    Finds all possible one letter edits of word_string:
    - Splitting word_string into two words at all character locations
    - Deleting one letter at all character locations
    - Switching neighbouring characters
    - Replacing a character with every alphabetical letter
    - Inserting all possible alphabetical characters between each character location including boundaries

    Returns all one letter edits as a set instance.
    '''
    if word_string is None:
        return {}
    elif isinstance(word_string, str):
        splits = [(word_string[:i], word_string[i:]) for i in range(len(word_string) + 1)]
        deletes = [L + R[1:] for L, R in splits if R]
        transposes = [L + R[1] + R[0] + R[2:] for L, R in splits if len(R) > 1]
        replaces = [L + c + R[1:] for L, R in splits if R for c in EN_ALPHABET]
        inserts = [L + c + R for L, R in splits for c in EN_ALPHABET]
        return set(deletes + transposes + replaces + inserts)
    else:
        raise InputError("string or none type variable not passed as argument to find_one_letter_edits")

def find_two_letter_edits(word_string):
    '''
    Finds all possible two letter edits of word_string:
    - Splitting word_string into two words at all character locations
    - Deleting one letter at all character locations
    - Switching neighbouring characters
    - Replacing a character with every alphabetical letter
    - Inserting all possible alphabetical characters between each character location including boundaries

    This can be seen as a reapplication of find_one_letter_edits to all words found via a first
    instantiation of find_one_letter_edits on word_string.

    Returns all two letter edits as a set instance.
    '''
    if word_string is None:
        return {}
    elif isinstance(word_string, str):
        return (e2 for e1 in find_one_letter_edits(word_string) for e2 in find_one_letter_edits(e1))
    else:
        raise InputError("string or none type variable not passed as argument to find_two_letter_edits")

def find_word_prob(word_string, word_total=sum(WORD_DISTRIBUTION.values())):
    '''
    Finds the relative probability of the word appearing given context of a base corpus.
    Returns this probability value as a float instance.
    '''
    if word_string is None:
        return 0
    elif isinstance(word_string, str):
        return WORD_DISTRIBUTION[word_string] / word_total
    else:
        raise InputError("string or none type variable not passed as argument to find_word_prob")

def validate_words(word_list):
    '''
    Checks for each edited word in word_list if that word is a valid english word.abs
    Returns all validated words as a set instance.
    '''
    if word_list is None:
        return {}
    elif isinstance(word_list, list):
        if not word_list:
            return {}
        else:
            return set(word for word in word_list if word in WORD_DISTRIBUTION)
    else:
        raise InputError("list variable not passed as argument to validate_words")
