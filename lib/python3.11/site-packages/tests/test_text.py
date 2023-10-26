'''unit tests for text module'''

from os import path
import sys
from unittest import TestCase

sys.path.insert(0, path.abspath(path.join(path.dirname(__file__), "..")))
import preprocessing.text as ptext
from preprocessing.text import (lowercase, remove_esc_chars, remove_unbound_punct,
                                remove_numbers)


class TestConvertHTMLEntitiesBadInput(TestCase):
    '''tests for bad input to convert_html_entities'''

    def test_non_string_input(self):
        '''convert_html_entities should fail given non-string input'''
        self.assertRaises(ptext.InputError, ptext.convert_html_entities, [])


class TestConvertHTMLEntitiesGoodInput(TestCase):
    '''tests for good input to convert_html_entities'''

    def test_expected_outcome(self):
        '''convert_html_entities should return expected output given known input'''
        self.assertEqual(ptext.convert_html_entities(""), "")
        self.assertEqual(ptext.convert_html_entities(None), "")
        self.assertEqual(ptext.convert_html_entities("&amp;"), "&")
        self.assertEqual(ptext.convert_html_entities('&quot;'), '"')


class TestConvertLigaturesBadInput(TestCase):
    '''tests for bad input to convert_ligatures'''

    def test_non_string_input(self):
        '''covert_ligatures should fail given non-string input'''
        self.assertRaises(ptext.InputError, ptext.convert_ligatures, [])


class TestConvertLigaturesGoodInput(TestCase):
    '''tests for good input to convert_ligatures'''

    def test_expected_outcome(self):
        '''convert_ligatures should provide expected output given known input'''
        self.assertEqual(ptext.convert_ligatures("Ꜳ"), "AA")
        self.assertEqual(ptext.convert_ligatures("ꜳ"), "aa")
        self.assertEqual(ptext.convert_ligatures("’"), "'")
        self.assertEqual(ptext.convert_ligatures("Œ"), "OE")
        self.assertEqual(ptext.convert_ligatures("ﬁ"), "fi")
        self.assertEqual(ptext.convert_ligatures(None), "")


class TestCorrectSpellingBadInput(TestCase):
    '''tests for bad input to correct_spelling'''

    def test_non_string_input(self):
        '''correct_spelling should fail given non-string input'''
        self.assertRaises(ptext.InputError, ptext.correct_spelling, [])


class TestCorrectSpellingGoodInput(TestCase):
    '''tests for good input to correct_spelling'''

    def test_expected_outcome(self):
        '''correct_spelling should provide expected outcome given known input'''
        self.assertEqual(ptext.correct_spelling("ten terts"), "ten terms")
        self.assertEqual(ptext.correct_spelling(None), "")


class TestCreateSentenceListBadInput(TestCase):
    '''tests for bad input to create_sentence_list'''

    def test_non_string_input(self):
        '''create_sentence_list should fail given non-string input'''
        self.assertRaises(ptext.InputError, ptext.create_sentence_list, [])


class TestCreateSentenceListGoodInput(TestCase):
    '''tests for good input to create_sentence_list'''

    def test_expected_outcome(self):
        '''create_sentence_list should provide expected sentence list given known input'''
        self.assertEqual(ptext.create_sentence_list("test sentence. another test sentence. 10.0 "
                                                    "test sentences."),
                         ['test sentence.', 'another test sentence.', '10.0 test sentences.'])


class TestKeywordTokenizeBadInput(TestCase):
    '''tests for bad input to keyword_tokenize'''

    def test_non_string_input(self):
        '''keyword_tokenize should fail given non-string input'''
        self.assertRaises(ptext.InputError, ptext.keyword_tokenize, [])


class TestKeywordTokenizeGoodInput(TestCase):
    '''tests for good input to keyword_tokenize'''

    def test_expected_outcome(self):
        '''keyword_tokenize should return expected output given known input'''
        self.assertEqual(ptext.keyword_tokenize(""), "")
        self.assertEqual(ptext.keyword_tokenize(None), "")
        self.assertEqual(ptext.keyword_tokenize("a test string"), "test string")


class TestLemmatizeBadInput(TestCase):
    '''tests for bad input to lemmatize'''

    def test_invalid_input(self):
        '''lemmatize should fail given invalid input'''
        self.assertRaises(ptext.InputError, ptext.lemmatize, [])


class TestLemmatizeGoodInput(TestCase):
    '''tests for good input to lemmatize'''

    def test_expected_outcome(self):
        '''lemmatize should return expected outcome given known input'''
        self.assertEqual(ptext.lemmatize("words"), "word")
        self.assertEqual(ptext.lemmatize("1"), "1")
        self.assertEqual(ptext.lemmatize("[]"), "[]")
        self.assertEqual(ptext.lemmatize(None), "")
        self.assertEqual(ptext.lemmatize(""), "")


class TestLowercaseBadInput(TestCase):
    '''tests for bad input to lowercase'''

    def test_non_string_input(self):
        '''lowercase should fail given non-string input'''
        self.assertRaises(ptext.InputError, ptext.lowercase, [])


class TestLowercaseGoodInput(TestCase):
    '''tests for good input to lowercase'''

    def test_expected_outcome(self):
        '''lowercase should return expected output given known input'''
        self.assertEqual(ptext.lowercase(""), "")
        self.assertEqual(ptext.lowercase(None), "")
        self.assertEqual(ptext.lowercase("A TesT StriNG"), "a test string")


class TestPreprocessTextBadInput(TestCase):
    '''tests for bad input to preprocess_text'''

    def test_non_string_input(self):
        '''preprocess_text should fail given non-string input'''
        self.assertRaises(ptext.InputError, ptext.preprocess_text, [], ["test"])

    def test_non_list_input(self):
        '''preprocess_text should fail given non-list input'''
        self.assertRaises(ptext.InputError, ptext.preprocess_text, "test", "test")

    def test_invalid_function(self):
        '''preprocess_text should fail given invalid function'''
        self.assertRaises(ptext.FunctionError, ptext.preprocess_text, "test", ["test"])


class TestPreprocessTextGoodInput(TestCase):
    '''tests for good input to preprocess_text'''

    def text_expected_outcome(self):
        '''preprocess_text should return expected outcome given known input'''
        self.assertEqual("test string", ptext.preprocess_text("Test\nString 1 ;.", [
            lowercase,
            remove_esc_chars,
            remove_numbers,
            remove_unbound_punct
        ]))


class TestRemoveEscCharsBadInput(TestCase):
    '''tests for bad input to remove_esc_chars'''

    def test_non_string_input(self):
        '''remove_esc_chars should fail given non-string input'''
        self.assertRaises(ptext.InputError, ptext.remove_esc_chars, [])


class TestRemoveEscCharsGoodInput(TestCase):
    '''tests for good input to remove_esc_chars'''

    def test_expected_outcome(self):
        '''remove_esc_chars should return expected output given known input'''
        self.assertEqual(ptext.remove_esc_chars(""), "")
        self.assertEqual(ptext.remove_esc_chars(None), "")
        self.assertEqual(ptext.remove_esc_chars("a\ntest\nstring"), "a test string")


class TestRemoveNumbersBadInput(TestCase):
    '''tests for bad input to remove_numbers'''

    def test_non_string_input(self):
        '''remove_numbers should fail given non-string input'''
        self.assertRaises(ptext.InputError, ptext.remove_numbers, [])


class TestRemoveNumbersGoodInput(TestCase):
    '''tests for good input to remove_numbers'''

    def test_expected_outcome(self):
        '''remove_numbers should return expected string given correct input'''
        self.assertEqual(ptext.remove_numbers(""), "")
        self.assertEqual(ptext.remove_numbers(None), "")
        self.assertEqual(ptext.remove_numbers("40 tests"), "tests")


class TestRemoveNumberWordsBadInput(TestCase):
    '''tests for bad input to remove_number_words'''

    def test_non_string_input(self):
        '''remove_number_words should fail given non-string input'''
        self.assertRaises(ptext.InputError, ptext.remove_number_words, [])


class TestRemoveNumberWordsGoodInput(TestCase):
    '''tests for good input to remove_number_words'''

    def test_expected_outcome(self):
        '''remove_number_words should return expected string given known input'''
        self.assertEqual(ptext.remove_number_words("one year i did two hour tests"),
                         "year i did hour tests")


class TestRemoveTimeWordsBadInput(TestCase):
    '''tests for bad input to remove_time_words'''

    def test_non_string_input(self):
        '''remove_time_words should fail given non-string input'''
        self.assertRaises(ptext.InputError, ptext.remove_time_words, [])


class TestRemoveTimeWordsGoodInput(TestCase):
    '''tests for good input to remove_time_words'''

    def test_expected_outcome(self):
        '''remove_time_words should return expected string given known input'''
        self.assertEqual(ptext.remove_time_words("one year i did two hour tests"),
                         "one i did two tests")


class TestRemoveUnboundPunctBadInput(TestCase):
    '''tests for bad input to remove_unbound_punct'''

    def test_non_string_input(self):
        '''remove_unbound_punct should fail given non-string input'''
        self.assertRaises(ptext.InputError, ptext.remove_unbound_punct, [])


class TestRemoveUnboundPunctGoodInput(TestCase):
    '''tests for good input to remove_unbound_punct'''

    def test_expected_outcome(self):
        '''remove_unbound_punct should return expected string given correct input'''
        self.assertEqual(ptext.remove_unbound_punct(""), "")
        self.assertEqual(ptext.remove_unbound_punct(None), "")
        self.assertEqual(ptext.remove_unbound_punct("'./'' a . test . string"), "a test string")


class TestRemoveURLsBadInput(TestCase):
    '''tests for bad input to remove_urls'''

    def test_non_string_input(self):
        '''remove_urls should fail given non-string input'''
        self.assertRaises(ptext.InputError, ptext.remove_urls, [])


class TestRemoveURLsGoodInput(TestCase):
    '''tests for good input to remove_urls'''

    def test_expected_outcome(self):
        '''remove_urls should return expected string given correct input'''
        self.assertEqual(ptext.remove_urls(""), "")
        self.assertEqual(ptext.remove_urls(None), "")
        self.assertEqual(ptext.remove_urls("http://example.com"), "")
