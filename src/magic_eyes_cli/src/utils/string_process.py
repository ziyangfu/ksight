"""
Module providing utilities about string processing
"""

class StringUtil:
    """
    Class acting as a namespace for string utility functions.
    Static methods know nothing about the class, class methods can access class variables.
    If a function needs to track state via class variables, define a new class for it.
    """

    STRING_ESCAPE_SEQS = [
        "'",    # ' character
        '"',    # " character
    ]

    @classmethod
    def get_escaped_string(cls, input_string, escape_seqs=None):
        """
        Processes the input string and returns a string that is between two (same) escape sequences.
        If no such sub-string satisfy this condition, then returns None.

        Args:
            input_string (str): Input string that should be processed

        Keyword Args:
            escape_seqs (List[str]): List of strings that should be interpreted as escape sequences.
                If not specified, cls.STRING_ESCAPE_SEQS is used.

        Returns:
            str | None : The string between two escape sequences (without the escape sequences) if found,
                else None
        """
        escaped_string = None
        esc_seqs = escape_seqs or cls.STRING_ESCAPE_SEQS

        for esc_seq in esc_seqs:
            # see https://docs.python.org/3/library/stdtypes.html#text-sequence-type-str if unclear
            # quick check if esc_seq found is to check if seperator is not empty

            # check for escape sequence from beginning
            before, sep_1, after_1 = input_string.partition(esc_seq) # pylint: disable=unused-variable
            if sep_1:
                # check for escape sequence in remaining part from end
                between, sep_2, after_2 = after_1.rpartition(esc_seq) # pylint: disable=unused-variable
                if sep_2:
                    escaped_string = between
                    # if an escaped string is found for a sequence, do not search with other sequences
                    break
        return escaped_string

    @staticmethod
    def get_kv_pair_from_string(string, seperator='='):
        """ return a key-value pair from string, seperated by seperator """
        fields = string.split(seperator)
        if len(fields) == 2:
            key = fields[0]
            value = fields[1]
        elif len(fields) > 2:
            # check if the option is actually a kv-pair but the value is an escaped string containing the seperator
            escaped_string = StringUtil.get_escaped_string(seperator.join(fields[1:]))
            if escaped_string:
                key = fields[0]
                value = escaped_string
            else:
                raise ValueError("String %s is not a key-value pair seperated by %s" % (string, seperator))
        else:
            raise ValueError("String %s is not a key-value pair seperated by %s" % (string, seperator))

        return key, value
