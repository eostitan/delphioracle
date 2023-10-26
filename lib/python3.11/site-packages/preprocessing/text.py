'''
Text pre-processing module:
'''


from preprocessing.errors import FunctionError, InputError
import preprocessing.spellcheck as spellcheck

import html
import json
from os import path
import re
import string

import nltk.data
nltk.data.path = [path.join(path.dirname(__file__), "data")]
from nltk.corpus import stopwords
from nltk.stem import WordNetLemmatizer
from nltk.tokenize import RegexpTokenizer


KEYWORD_TOKENIZER = RegexpTokenizer(r'\b[\w.\/,-]+\b|[-.,\/()]')
LEMMATIZER = WordNetLemmatizer()
LIGATURES = json.load(open(path.join(path.dirname(__file__), "data/latin_characters.json"), "r"))
NUMBER_WORDS = [NUMBER_WORD.replace("\n", "") for NUMBER_WORD in open(path.join(path.dirname(__file__), "data/word_numbers.txt"), "r").readlines()]
PUNCT = string.punctuation
STOPWORDS = stopwords.words("english")
SENTENCE_TOKENIZER = nltk.data.load("tokenizers/punkt/english.pickle")
TIME_WORDS = [TIME_WORD.replace("\n", "") for TIME_WORD in open(path.join(path.dirname(__file__), "data/word_time.txt"), "r").readlines()]


#functions
def convert_html_entities(text_string):
    '''
    Converts HTML5 character references within text_string to their corresponding unicode characters
    and returns converted string as type str.

    Keyword argument:

    - text_string: string instance

    Exceptions raised:

    - InputError: occurs should a non-string argument be passed
    '''
    if text_string is None or text_string == "":
        return ""
    elif isinstance(text_string, str):
        return html.unescape(text_string).replace("&quot;", "'")
    else:
        raise InputError("string not passed as argument for text_string")

def convert_ligatures(text_string):
    '''
    Coverts Latin character references within text_string to their corresponding unicode characters
    and returns converted string as type str.

    Keyword argument:

    - text_string: string instance

    Exceptions raised:

    - InputError: occurs should a string or NoneType not be passed as an argument
    '''
    if text_string is None or text_string == "":
        return ""
    elif isinstance(text_string, str):
        for i in range(0, len(LIGATURES)):
            text_string = text_string.replace(LIGATURES[str(i)]["ligature"], LIGATURES[str(i)]["term"])
        return text_string
    else:
        raise InputError("none type or string not passed as an argument")

def correct_spelling(text_string):
    '''
    Splits string and converts words not found within a pre-built dictionary to their
    most likely actual word based on a relative probability dictionary. Returns edited
    string as type str.

    Keyword argument:

    - text_string: string instance

    Exceptions raised:

    - InputError: occurs should a string or NoneType not be passed as an argument
    '''
    if text_string is None or text_string == "":
        return ""
    elif isinstance(text_string, str):
        word_list = text_string.split()
        spellchecked_word_list = []
        for word in word_list:
            spellchecked_word_list.append(spellcheck.correct_word(word))
        return " ".join(spellchecked_word_list)
    else:
        raise InputError("none type or string not passed as an argument")

def create_sentence_list(text_string):
    '''
    Splits text_string into a list of sentences based on NLTK's english.pickle tokenizer, and
    returns said list as type list of str.

    Keyword argument:

    - text_string: string instance

    Exceptions raised:

    - InputError: occurs should a non-string argument be passed
    '''
    if text_string is None or text_string == "":
        return []
    elif isinstance(text_string, str):
        return SENTENCE_TOKENIZER.tokenize(text_string)
    else:
        raise InputError("non-string passed as argument for create_sentence_list")

def keyword_tokenize(text_string):
    '''
    Extracts keywords from text_string using NLTK's list of English stopwords, ignoring words of a
    length smaller than 3, and returns the new string as type str.

    Keyword argument:

    - text_string: string instance

    Exceptions raised:

    - InputError: occurs should a non-string argument be passed
    '''
    if text_string is None or text_string == "":
        return ""
    elif isinstance(text_string, str):
        return " ".join([word for word in KEYWORD_TOKENIZER.tokenize(text_string) if word not in STOPWORDS and len(word) >= 3])
    else:
        raise InputError("string not passed as argument for text_string")

def lemmatize(text_string):
    '''
        Returns base from of text_string using NLTK's WordNetLemmatizer as type str.

        Keyword argument:

        - text_string: string instance

        Exceptions raised:

        - InputError: occurs should a non-string argument be passed
    '''
    if text_string is None or text_string == "":
        return ""
    elif isinstance(text_string, str):
        return LEMMATIZER.lemmatize(text_string)
    else:
        raise InputError("string not passed as primary argument")

def lowercase(text_string):
    '''
    Converts text_string into lowercase and returns the converted string as type str.

    Keyword argument:

    - text_string: string instance

    Exceptions raised:

    - InputError: occurs should a non-string argument be passed
    '''
    if text_string is None or text_string == "":
        return ""
    elif isinstance(text_string, str):
        return text_string.lower()
    else:
        raise InputError("string not passed as argument for text_string")

def preprocess_text(text_string, function_list):
    '''
    Given each function within function_list, applies the order of functions put forward onto
    text_string, returning the processed string as type str.

    Keyword argument:

    - function_list: list of functions available in preprocessing.text
    - text_string: string instance

    Exceptions raised:
    
    - FunctionError: occurs should an invalid function be passed within the list of functions
    - InputError: occurs should text_string be non-string, or function_list be non-list
    '''
    if text_string is None or text_string == "":
        return ""
    elif isinstance(text_string, str):
        if isinstance(function_list, list):
            for func in function_list:
                try:
                    text_string = func(text_string)
                except (NameError, TypeError):
                    raise FunctionError("invalid function passed as element of function_list")
                except:
                    raise
            return text_string
        else:
            raise InputError("list of functions not passed as argument for function_list")
    else:
        raise InputError("string not passed as argument for text_string")

def remove_esc_chars(text_string):
    '''
    Removes any escape character within text_string and returns the new string as type str.

    Keyword argument:

    - text_string: string instance

    Exceptions raised:

    - InputError: occurs should a non-string argument be passed
    '''
    if text_string is None or text_string == "":
        return ""
    elif isinstance(text_string, str):
        return " ".join(re.sub(r'\\\w', "", text_string).split())
    else:
        raise InputError("string not passed as argument")

def remove_numbers(text_string):
    '''
    Removes any digit value discovered within text_string and returns the new string as type str.

    Keyword argument:

    - text_string: string instance

    Exceptions raised:

    - InputError: occurs should a non-string argument be passed
    '''
    if text_string is None or text_string == "":
        return ""
    elif isinstance(text_string, str):
        return " ".join(re.sub(r'\b[\d.\/,]+', "", text_string).split())
    else:
        raise InputError("string not passed as argument")

def remove_number_words(text_string):
    '''
    Removes any integer represented as a word within text_string and returns the new string as
    type str.

    Keyword argument:

    - text_string: string instance

    Exceptions raised:

    - InputError: occurs should a non-string argument be passed
    '''
    if text_string is None or text_string == "":
        return ""
    elif isinstance(text_string, str):
        for word in NUMBER_WORDS:
            text_string = re.sub(r'[\S]*\b'+word+r'[\S]*', "", text_string)
        return " ".join(text_string.split())
    else:
        raise InputError("string not passed as argument")

def remove_time_words(text_string):
    '''
    Removes any word associated to time (day, week, month, etc.) within text_string and returns the
    new string as type str.

    Keyword argument:

    - text_string: string instance

    Exceptions raised:

    - InputError: occurs should a non-string argument be passed
    '''
    if text_string is None or text_string == "":
        return ""
    elif isinstance(text_string, str):
        for word in TIME_WORDS:
            text_string = re.sub(r'[\S]*\b'+word+r'[\S]*', "", text_string)
        return " ".join(text_string.split())
    else:
        raise InputError("string not passed as argument")

def remove_unbound_punct(text_string):
    '''
    Removes all punctuation unattached from a non-whitespace or attached to another punctuation
    character unexpectedly (e.g. ".;';") within text_string and returns the new string as type str.

    Keyword argument:

    - text_string: string instance

    Exceptions raised:

    - InputError: occurs should a non-string argument be passed
    '''
    if text_string is None or text_string == "":
        return ""
    elif isinstance(text_string, str):
        return " ".join(re.sub(r''.join([r'[', PUNCT, r'][', PUNCT, r']+|\B[', PUNCT, r']+']), "",
                               text_string).split())
    else:
        raise InputError("string not passed as argument")

def remove_urls(text_string):
    '''
    Removes all URLs within text_string and returns the new string as type str.

    Keyword argument:

    - text_string: string instance

    Exceptions raised:

    - InputError: occurs should a non-string argument be passed
    '''
    if text_string is None or text_string == "":
        return ""
    elif isinstance(text_string, str):
        return " ".join(re.sub(r'http\S+', "", text_string).split())
    else:
        raise InputError("string not passed as argument")

def remove_whitespace(text_string):
    '''
    Removes all whitespace found within text_string and returns new string as type str.

    Keyword argument:

    - text_string: string instance

    Exceptions raised:

    - InputError: occurs should a string or NoneType not be passed as an argument
    '''
    if text_string is None or text_string == "":
        return ""
    elif isinstance(text_string, str):
        return " ".join(text_string.split())
    else:
        raise InputError("none type or string not passed as an argument")
