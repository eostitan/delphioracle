'''unit tests for spellcheck module'''

from os import path
import sys
from unittest import TestCase

sys.path.insert(0, path.abspath(path.join(path.dirname(__file__), "..")))
import preprocessing.spellcheck as pspell


class TestCorrectWordBadInput(TestCase):
    '''tests for bad input to correct_word'''

    def test_non_string_input(self):
        '''correct_word should fail given non-string input'''
        self.assertRaises(pspell.InputError, pspell.correct_word, [])


class TestCorrectWordGoodInput(TestCase):
    '''tests for good input to correct_word'''

    def test_expected_outcome(self):
        '''correct_word should provide expected outcome given known input'''
        self.assertEqual(pspell.correct_word("terts"), "terms")
        self.assertEqual(pspell.correct_word(None), "")


class TestFindCandidatesBadInput(TestCase):
    '''tests for bad input to find_candidates'''

    def test_non_string_input(self):
        '''find_candidates should fail given non-string input'''
        self.assertRaises(pspell.InputError, pspell.find_candidates, [])


class TestFindCandidatesGoodInput(TestCase):
    '''tests for good input to find_candidates'''

    def test_expected_outcome(self):
        '''find_candidates should fail given non-string input'''
        self.assertEqual(pspell.find_candidates(None), {})
        self.assertEqual(pspell.find_candidates("abcdefghi"), {"abcdefghi"})
        self.assertEqual(pspell.find_candidates("test"), {"test"})
        self.assertEqual(pspell.find_candidates("terts"), {
            "teres",
            "tests",
            "tents",
            "terms",
            "texts"
        })


class TestFindOneLetterEditsBadInput(TestCase):
    '''tests for bad input to find_one_letter_edits'''

    def test_non_string_input(self):
        '''find_one_letter_edits should fail given non-string input'''
        self.assertRaises(pspell.InputError, pspell.find_one_letter_edits, [])


class TestFindOneLetterEditsGoodInput(TestCase):
    '''tests for good input to find_one_letter_edits'''

    def test_expected_outcome(self):
        '''find_one_letter_edits should return expected outcome given known input'''
        expected_edits = open(path.join(path.dirname(__file__), "find_one_letter_edits_test.txt"), "r").read().split(",")
        test_edits = list(pspell.find_one_letter_edits("a"))
        self.assertEqual(len(test_edits), len(expected_edits))
        self.assertEqual(pspell.find_one_letter_edits(None), {})


class TestFindTwoLetterEditsBadInput(TestCase):
    '''tests for bad input to find_two_letter_edits'''

    def test_non_string_input(self):
        '''find_two_letter_edits should fail given non-string input'''
        self.assertRaises(pspell.InputError, pspell.find_two_letter_edits, [])


class TestFindTwoLetterEditsGoodInput(TestCase):
    '''tests for good input to find_two_letter_edits'''

    def test_expected_outcome(self):
        '''find_two_letter_edits should return expected outcome given known input'''
        expected_edits = open(path.join(path.dirname(__file__), "find_two_letter_edits_test.txt"), "r").read().split(",")
        test_edits = list(pspell.find_two_letter_edits("a"))
        self.assertEqual(len(test_edits), len(expected_edits))
        self.assertEqual(pspell.find_two_letter_edits(None), {})


class TestFindWordProbBadInput(TestCase):
    '''tests for bad input to find_word_prob'''

    def test_non_string_input(self):
        '''find_word_prob should fail given non-string input'''
        self.assertRaises(pspell.InputError, pspell.find_word_prob, [])


class TestFindWordProbGoodInput(TestCase):
    '''tests for good input to find_word_prob'''

    def test_expected_outcome(self):
        '''find_word_prob should return expected outcome given known input'''
        self.assertEqual(pspell.find_word_prob("terms"), 0.00013266582107145577)
        self.assertEqual(pspell.find_word_prob(None), 0)
        self.assertEqual(pspell.find_word_prob("reliable"), 1.7927813658304835e-05)


class TestValidateWordsBadInput(TestCase):
    '''tests for bad input to validate_words'''
    
    def test_non_list_input(self):
        '''validate_words should fail given non-list input'''
        self.assertRaises(pspell.InputError, pspell.validate_words, "")


class TestValidateWordsGoodInput(TestCase):
    '''tests for good input to validate_words'''

    def test_expected_outcome(self):
        self.assertEqual(pspell.validate_words(["test", "terts"]), {"test"})
        self.assertEqual(pspell.validate_words(None), {})
        self.assertEqual(pspell.validate_words([]), {})
        
