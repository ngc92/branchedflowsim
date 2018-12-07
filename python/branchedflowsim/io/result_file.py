import os
from .data_spec import DataSpec


class ResultFile(object):
    """
    This class manages the loading, manipulation and re-saving of results produced by the observers
    of the tracer program. Derived classes can specify a `_FILE_HEADER_` class variable which will be
    used to validate files from which data is loaded, and written to save file.

    Furthermore, derived classes must have a _SPEC_ class attribute, that contains the specification
    for the data that is contained in this result file.
    """

    def __new__(cls, *args, **kwargs):
        if not hasattr(cls, "_SPEC_"):
            raise ValueError("No _SPEC_ found in class %s" % cls)
        return super(ResultFile, cls).__new__(cls, *args, **kwargs)

    def __init__(self, source=None):
        """
        Initializes the ResultFile from data provided by `source`.
        If `source` is set to `None` no data will be initialized, if a dict is supplied data will be read
        directly from the dict. Can also supply a path, in which case, if the file exists
        it will be read from that path, and otherwise, if there is a default `_FILE_NAME_`
        specified in the class, this will be appended to the path and the file will be read
        from there. If neither is possible an IOError will be raised.
        
        :param str|BinaryIO|dict source: The source from which to construct the ResultFile.
        """
        if source is not None:
            if isinstance(source, (str, unicode)):
                if os.path.isfile(source):
                    self.from_file(source)
                elif hasattr(self, "_FILE_NAME_"):
                    self.from_file(os.path.join(source, self._FILE_NAME_))
                else:
                    raise IOError("Could not open result {}".format(source))
            elif isinstance(source, dict):
                self.from_dict(source)
            else:
                self.from_file(source)

    @property
    def spec(self):
        """
        The `_SPEC_` object of this class.
        Contains basic information on how to read/write the results, the attributes
        given in this class and the type of reduction to perform for each attribute.
        """
        return getattr(self, "_SPEC_")

    def from_file(self, source_file):
        """
        Loads the data from a file, that can be either specified as a filename string, or an already opened file.
        This checks the file header (if specified) and then reads all data according to the objects `_SPEC_`.
        Additional postprocessing can be performed with a user defined `_from_file` function, then the gathered
        data will be passed on to the `from_dict` function.
        
        :raises IOError: if the file header does not match the specified `_FILE_HEADER_`.
        """

        if isinstance(source_file, (str, unicode)):
            with open(source_file, "rb") as opened_file:
                self.from_file(opened_file)
                return

        # if a _FILE_HEADER_ is specified, check that it is present
        if hasattr(self, "_FILE_HEADER_"):
            header = self._FILE_HEADER_  # type: str
            file_start = source_file.read(len(header))
            if header != file_start:
                raise IOError("File header {} differs from expected file header {}".format(file_start, header))

        # now read the file as far as _SPEC_ goes
        data = {}
        for spec in self.spec:  # type: DataSpec
            spec.read(source_file, data)

        self._from_file(source_file, data)
        self.from_dict(data)

    def from_dict(self, data):
        """
        Sets the `ResultFile`'s attributes according to the data specified in `data`.
        Only attributes for specs that have `is_attr` set will be set. There is no
        verification that the data actually corresponds (in data type and shape) to
        the specification in `_SPEC_`.
        
        :param dict data: A dictionary containing data where the keys follow the name
                          entries of the `_SPEC_`.
        :return: nothing.
        :raises TypeError: If `data` is not a dict.
        """
        if not isinstance(data, dict):
            raise TypeError("from_dict expects a dict argument, got {}".format(data))

        for spec in self.spec:  # type: DataSpec
            if spec.is_attr:
                setattr(self, spec.name, data[spec.name])

        self._from_dict(data)

    def to_file(self, target_file):
        """
        Write the data to a file. `target_file` can be either an opened file or a filename.
        """
        if isinstance(target_file, str):
            with open(target_file, "wb") as opened_file:
                self.to_file(opened_file)
                return

        # if a _FILE_HEADER_ is specified, write it
        if hasattr(self, "_FILE_HEADER_"):
            header = self._FILE_HEADER_  # type: str
            target_file.write(header)

        data = {}
        for spec in self.spec:  # type: DataSpec
            if spec.is_attr:
                data[spec.name] = getattr(self, spec.name)

        self._to_file(data)

        for spec in self.spec:
            spec.write(target_file, data)

    def reduce(self, other):
        """
        This operation reduces the results of two different ResultFile objects into a single one. This may
        not be possible for all types of results, so an AssertionError may be raised. If for `other`
        a `None` value is passed, this function returns the current results (`self`) unchanged to allow
        for easy chaining of reductions as in::

          result = None
          for next_result in generate_results():
              result = next_result.reduce(result)
        
        The operation does modify `self`, and returns a new reference just for convenience.
        
        :param ResultFile other: the result file to be merged with this one, of the same type as `self`, or `None`.
        :return: `self`.
        :raises: AssertionError, if a reduction is deactivated by setting it to `Reductions.fail` \
                 for one of the attributes.
        """
        if other is None:
            return self
        if not isinstance(other, type(self)):
            raise TypeError("Cannot reduce {} and {}".format(type(self), type(other)))

        data = {}
        for spec in self.spec:  # type: DataSpec
            if not spec.is_attr:
                continue

            try:
                data[spec.name] = spec.reduction(getattr(self, spec.name), getattr(other, spec.name))
            except AssertionError as E:
                raise AssertionError("Assertion triggered when reducing %s: %s" % (spec.name, E.message))

        self.from_dict(data)
        return self

    def _from_file(self, source_file, data):
        """
        Override this function for custom behaviour when reading data from file. This allows you to
        fill in additional entries of `data`.
        """
        pass

    def _from_dict(self, data):
        """
        Override this function to customize the `from_dict` behaviour (e.g. setting additional attributes
        that can be calculated from the values in `data`).
        """
        pass

    def _to_file(self, data):
        """
        Override this function if your `_SPEC_` contains specification with `is_attr` set to `False`.
        These cannot be filled in automatically when saving to file, so you need to do that yourself in
        this function by inserting them into the `data` dict.
        """
        pass


def load_result(file_or_filename):
    """
    Load a ResultFile object from a given file. The correct subclass is chosen according to the first bytes in the file.
    :param str|BinaryIO file_or_filename: A filename, or an opened file object.
    :return:
    """
    if isinstance(file_or_filename, (str, unicode)):
        return load_result(open(file_or_filename, "rb"))

    file_ = file_or_filename  # type: file
    # iterate over all
    for type_ in ResultFile.__subclasses__():
        if not hasattr(type_, "_FILE_HEADER_"):
            continue
        header = type_._FILE_HEADER_
        init_bytes = file_.read(len(header))
        file_.seek(0)
        if init_bytes == header:
            return type_(file_)

    raise IOError("Could not find a suitable ResultsFile subtype for file %s" % file_)
