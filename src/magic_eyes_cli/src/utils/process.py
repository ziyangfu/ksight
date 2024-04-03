"""
Module providing utilites for processes
"""

import os
import logging
import re
import subprocess

class ProcessError(RuntimeError):
    """ Simple custom exception to indicate an error during process execution """
    def __init__(self, message, process_executor):
        """
        Args:
            message(str): Exception message
            executor(ProcessExecutor): the process executor object that caused an exception
        """
        # call the constructor of the base class
        super().__init__(message)
        self.executor = process_executor


class ProcessExecutor:
    """
    Class providing utility functions for executing a process,
    such as:
        logging,
        getting and/or checking the return code,
        storing and/or filtering the output.
    """

    def __init__(self, command, shell=True, cwd=None):
        """
        Constructor initializing instance variables.

        Args:
            command (str or List): is either a string or a list of strings,

        Keyword Args:
            shell (bool): indicating if the command should be executed through the shell.
                without the shell, environment variables like ~ and $EDITOR are not expanded,
                aliases are not effective etc.
            cwd (str): the command is executed in the directory given by cwd,
                if None, the command is executed in the current directory
            logger (Callable): logger object, use None to disable logging

        Attributes:
            All private attributes are wrapped by properties and documented there.

        Create an instance by specifying the execution options of the command.

        >>> p1 = ProcessExecutor(['ls', '-l', '~/Documents'])
        >>> p2 = ProcessExecutor('pwd', cwd='..')
        """
        self._command = ' '.join(command) if isinstance(command, list) else str(command)
        self._use_shell = shell
        self._cwd = cwd or os.getcwd()
        self._return_code = None
        self._output_log = None

    # Properties
    @property
    def command(self):
        """ The command executed by the instance """
        return self._command

    @property
    def is_shell_used(self):
        """
        Is the command executed through the shell, see subprocess.Popen
        """
        return self._use_shell

    @property
    def cwd(self):
        """ The directory where the command will be executed """
        return self._cwd

    @property
    def return_code(self):
        """ Return code of the executed command, None if the command is not yet executed """
        return self._return_code

    @property
    def output_log(self):
        """
        Complete output of the executed command stored line by line as a list of strings.
        None if the command is not yet executed or the output is not stored
        """
        return self._output_log

    # Private methods
    def _gen_execute_process(self):
        """
        Executes the command parameterized by the instance variables,
        stores the exit code into an instance variable.
        Logs the output line by line.

        Yields:
            A generator that produces the output line by line

        The process executing the command is run uninterrupted from start to finish.
        The generator instead of a function only makes a difference about how the
        output of the command is stored. Generator is more memory efficient than
        storing the entire output as a list.

        >>> for line in self._gen_execute_process():
        >>>     print("Processing line: '%s'" % line)
        """
        print("Executing process '%s'", self.command)
        process_handle = subprocess.Popen(self.command, shell=self.is_shell_used, cwd=self.cwd,
                                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        while True:
            out_line = process_handle.stdout.readline().decode('utf-8', errors='replace').rstrip()
            if process_handle.poll() is not None and out_line == "":
                self._return_code = process_handle.returncode
                break
            if out_line:
                print(out_line)
                yield out_line

    # Public methods
    def execute(self, store_output=False,
                match_regex=None, regex_match_handler=None):
        """
        Executes the command, optionally stores the output log
        and/or redirects matching output lines to a handler.

        Keyword Args:
            log_function (Callable): this callable is used to log the execution
            store_output (bool): if True, stores the output log in an instance variable
            match_regex (str): each line is searched with this regex pattern,
                matches are passed to regex_match_handler as a re.match object.
                if None, every line is a match and passed as a string
            regex_match_handler (Callable): callable object which accepts one argument

        Returns:
            The exit code of the executed process

        It is used after initializing a ProcessExecutor object.

        >>> p = ProcessExecutor('seq 1 3')
        >>> return_code = p.execute(store_output_log=True)
        >>> assert return_code == p.return_code
        >>> output_log = p.output_log
        """
        match_handler = regex_match_handler or (lambda match: None)
        compiled_regex = re.compile(match_regex) if match_regex else None
        self._output_log = []

        for output_line in self._gen_execute_process():
            self._output_log.append(output_line)
            if compiled_regex:
                match = compiled_regex.match(output_line)
                if match:
                    match_handler(match)
            else:
                match_handler(output_line)

        if not store_output:  # free up memory
            self._output_log = list()

        return self._return_code

    def execute_check_return(self, expected_return_code=os.EX_OK, # pylint: disable=too-many-arguments
                             error_description=None, store_output=False,
                             match_regex=None, regex_match_handler=None):
        """
        Convenience wrapper around ProcessExecutor.execute method.

        Keyword Args:
            expected_return_code (int): compared to the exit code of the executed command
            error_description (str): the message would be displayed with the raised exception

        Returns:
            True if the return code of the command is expected_return_code
            False otherwise

        Raises:
            RuntimeError if the return code of the command is not the expected value

        Can be used to quickly check a resource, connection etc.

        >>> HOST_NAME = 'google.com'
        >>> p = ProcessExecutor(['ping', '-q', '-c', '1', HOST_NAME])
        >>> p.execute_check_return(error_description="%s is not accessible" % HOST_NAME)
        """
        result = False
        # Let the method store the output in case an exception needs to be thrown
        return_code = self.execute(store_output=True,
                                   match_regex=match_regex,
                                   regex_match_handler=regex_match_handler)
        if return_code == expected_return_code:
            result = True
            if not store_output:  # free up memory
                self._output_log = list()
        else:
            def_error_msg = ('The command %s expected to return with code %d but returned with %d'
                             % (self.command, expected_return_code, return_code))
            error_msg = error_description or def_error_msg
            raise ProcessError(error_msg, self)
        return result


# Examples
if __name__ == '__main__':

    def line_handler(line):
        """ demo function """
        logging.info("Processing output line: '%s' of type %s", line, type(line))

    def regex_handler(match):
        """ demo function """
        logging.info("Processing regex match: '%s' of type %s",
                     match.group(0), type(match))

    logging.basicConfig(level=logging.DEBUG)

    # initialization and execution are separated
    SEQ_PROCESS = ProcessExecutor('seq 1 3')

    # same ProcessExecutor instance can be executed many times
    SEQ_PROCESS.execute(regex_match_handler=line_handler)

    # with different output handling options
    SEQ_PROCESS.execute(match_regex=r"^2", regex_match_handler=regex_handler)
    SEQ_PROCESS.execute(store_output=True, log_function=None)
    logging.info("Stored output: '%s'", SEQ_PROCESS.output_log)

    # convenience wrapper
    HOST_NAME = 'google.com.xyzt'
    PING_GOOGLE_PROCESS = ProcessExecutor(['ping', '-q', '-c', '1', HOST_NAME])
    PING_GOOGLE_PROCESS.execute_check_return(error_description="%s is not accessible" % HOST_NAME)
