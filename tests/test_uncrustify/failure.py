# Exceptions when a test fails.
#
# * @author  Matthew Woehlke    June 2018
#


# =============================================================================
class Failure(Exception):
    pass

# =============================================================================
class TestDeclarationParseError(Failure):
    # -------------------------------------------------------------------------
    def __init__(self, test_suite, line_number):
        self.test_suite = test_suite
        self.line_number = line_number

    # -------------------------------------------------------------------------
    def __str__(self):
        return 'Error parsing line {!r} from the {!r} test suite'.format(
            self.line_number, self.test_suite)


# =============================================================================
class ExecutionFailure(Failure):
    # -------------------------------------------------------------------------
    def __init__(self, exception):
        self.exception = exception

    # -------------------------------------------------------------------------
    def __str__(self):
        return str(self.exception)


# =============================================================================
class MissingFailure(Failure):
    # -------------------------------------------------------------------------
    def __init__(self, exception, missing_path):
        self.exception = exception
        self.missing_path = missing_path

    # -------------------------------------------------------------------------
    def __str__(self):
        return 'Expected output file not found: {!r}'.format(self.missing_path)


# =============================================================================
class MismatchFailure(Failure):
    # -------------------------------------------------------------------------
    def __init__(self, expected, actual):
        self.expected_path = expected
        self.actual_path = actual

    # -------------------------------------------------------------------------
    def __str__(self):
        return 'Output {!r} does not match expected output {!r}'.format(
            self.actual_path, self.expected_path)


# =============================================================================
class UnstableFailure(Failure):
    # -------------------------------------------------------------------------
    def __init__(self, expected, actual):
        self.expected_path = expected
        self.actual_path = actual

    # -------------------------------------------------------------------------
    def __str__(self):
        return 'Output {!r} does not match expected output {!r}'.format(
            self.actual_path, self.expected_path)

# =============================================================================
class UnexpectedlyPassingFailure(Failure):
    # -------------------------------------------------------------------------
    def __init__(self, expected, actual):
        self.expected_path = expected
        self.actual_path = actual

    # -------------------------------------------------------------------------
    def __str__(self):
        return 'Output {!r} unexpectedly matches expected output {!r}'.format(
            self.actual_path, self.expected_path)
