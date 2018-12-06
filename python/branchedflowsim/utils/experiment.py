import shutil

import datetime
import logging
import os
import json
import numpy as np
from abc import ABCMeta, abstractmethod, abstractproperty
import branchedflowsim.config


_logger = logging.getLogger(__name__)


class ConfigJsonEncoder(json.JSONEncoder):
    def default(self, obj):
        if isinstance(obj, np.ndarray):
            if obj.size > branchedflowsim.config.MAXIMUM_NUMPY_JSON:
                raise ValueError("Configuration dict contains numpy array with more than %i entries. "
                                 "If you really want to serialize this many elements to json, increase "
                                 "branchedflowsim.config.MAXIMUM_NUMPY_JSON" % branchedflowsim.config.MAXIMUM_NUMPY_JSON)
            return list(obj)

        if hasattr(obj, "as_dict"):
            return obj.as_dict
        # if we do not know how to handle the object, use its dict.
        return obj.__dict__


class PlotFunction(object):
    def __init__(self, plot_fn):
        self.plot_fn = plot_fn
        self.format = "pdf"
        self._data_cache = None

    def _set_cached_data(self, data):
        self._data_cache = data

    def __call__(self):
        self.plot_fn(self._data_cache)

    def save(self, filename):
        import matplotlib.pyplot as plt
        file_name = filename + "." + self.format

        if not os.path.exists(os.path.dirname(file_name)):
            os.makedirs(os.path.dirname(file_name))

        if not os.path.exists(file_name):
            fig = plt.figure(figsize=(20, 20), dpi=300)
            self()
            plt.savefig(file_name)
            plt.close(fig)

    def interactive(self):
        import matplotlib.pyplot as plt
        fig = plt.figure()
        self()
        plt.show()
        plt.close(fig)


def equal_by_json(a, b):
    str_a = json.dumps(a, sort_keys=True, cls=ConfigJsonEncoder)
    str_b = json.dumps(b, sort_keys=True, cls=ConfigJsonEncoder)
    return str_a == str_b


class Experiment(object):
    """
    `Experiment` is a utility class that manages the persistence of calculation results. They can be persisted for the
    program run, on disk, or immediately discarded.
    """

    __metaclass__ = ABCMeta

    PERSIST_DISK = "DISK"
    PERSIST_RAM  = "RAM"
    PERSIST_NO   = "NO"

    DEFAULT_DATA_DIR = "."

    def __init__(self, name, data_directory=None, persistence=None, dependencies=None):
        self._persistence = persistence
        self._data_directory = data_directory
        self.name = name
        self._dependencies = dependencies or {}
        self._data = None

    @property
    def persistence(self):
        return self._persistence or Experiment.PERSIST_DISK

    @property
    def data_directory(self):
        return self._data_directory or os.path.join(self.DEFAULT_DATA_DIR, self.name)

    @abstractmethod
    def generate_data(self, dependencies):
        raise NotImplementedError()

    def add_dependency(self, dependency, key=None):
        if key is None:
            key = dependency.name
        assert key not in self._dependencies, key
        assert isinstance(dependency, Experiment)
        self._dependencies[key] = dependency

    @abstractproperty
    def config(self):
        """
        Config determines the "configuration" of the experiment, that is the hyperparameters. If config changes, this
        should indicate that the data has to be regenerated. 
        :return: A json serializable dict.
        """
        raise NotImplementedError

    @property
    def data(self):
        """
        Calculate or load the results for this experiment and return them. Results are cached, so multiple calls
        will just return the same reference.
        :return: The result data of this experiment.
        """
        if self._data is None:
            self._data = self._generate_or_load_data()
        return self._data
    
    @property
    def timestamp(self):
        """
        Returns the date and time at which the data was generated. If no data could be found on disk,
        `datetime.now` is returned as the next call to `.data` will generate data.
        :return: 
        """
        json_path = os.path.join(self.data_directory, "data.json")
        if not os.path.exists(json_path):
            return datetime.datetime.now()

        with open(json_path, "r") as json_file:
            meta = json.load(json_file)
        return datetime.datetime.strptime(meta["__timestamp__"], "%Y-%m-%d %H:%M:%S")

    @property
    def is_config_up_to_date(self):
        config_path = os.path.join(self.data_directory, "config.json")
        if not os.path.exists(config_path):
            return False
        with open(config_path, "r") as json_file:
            old_config = json.load(json_file)
        return equal_by_json(old_config, self.config)

    def _generate_or_load_data(self, regenerate=False):
        """
        If data already exists on disk, load that, otherwise generate new data.
        In the latter case the new data is saved to the file system.
        :return: The loaded or generated data.
        """
        generate_data = not os.path.exists(self.data_directory) or regenerate

        # check whether any dependencies are newer than our data. in that case we need to regenerate the data.
        our_time = self.timestamp
        for dep in self._dependencies.values():  # type: Experiment
            their_time = dep.timestamp
            if our_time < their_time or not dep.is_config_up_to_date:
                generate_data = True
                _logger.info("Regenerating data for experiment '%s' because dependency '%s' is out of date.", self.name,
                             dep.name)

        # check whether the configuration as changed.
        if not self.is_config_up_to_date:
            _logger.info("Regenerating data for experiment '%s' because configuration is out of date.", self.name)
            generate_data = True

        if generate_data:
            # clean up all old data
            if os.path.exists(self.data_directory):
                shutil.rmtree(self.data_directory)

            # if the data does not exist, generate and save
            deps = {key: self._dependencies[key].data for key in self._dependencies}
            data = self.generate_data(deps)

            # ensure existence of data directory
            os.makedirs(self.data_directory)

            save_data = data
            if not isinstance(save_data, dict):
                save_data = {"__data__": save_data}

            # add a date key to the data
            save_data["__timestamp__"] = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")

            if self.persistence is Experiment.PERSIST_DISK:
                self._save_data(save_data)

        else:
            # otherwise, try to load.
            data = self._load_data()

            # if we converted to dict for saving, we revert that here.
            if "__data__" in data:
                data = data["__data__"]

        return data

    def _load_data(self):
        """
        Load data from given directory/file name combination. This function assumes that the data
        exists and raises an `IOException` otherwise.
        :return:
        """
        # first load the metadata
        with open(os.path.join(self.data_directory, "data.json"), "r") as json_file:
            meta = json.load(json_file)

        def load_blobs(data):
            for key in data:
                value = data[key]
                if isinstance(value, dict):
                    if "__blob__" in value:
                        type_ = value["__type__"]
                        path = os.path.join(self.data_directory, value["__blob__"])
                        if type_ == "numpy":
                            data[key] = np.load(path)
                        elif type_ == "ResultFile":
                            from branchedflowsim.io.result_file import load_result
                            data[key] = load_result(path)
                        elif type_ == "Plot":
                            # plot will point to a file. this will not be loaded
                            pass
                        else:
                            raise IOError("Unsupported type %s" % type_)
                    else:
                        load_blobs(value)

        load_blobs(meta)

        return meta

    def _save_data(self, data):
        meta, big = self._split_data(data, "blobs")

        # metadata is saved as json
        with open(os.path.join(self.data_directory, "data.json"), "w") as json_file:
            json.dump(meta, json_file, indent=2)

        # config is saved as json
        with open(os.path.join(self.data_directory, "config.json"), "w") as json_file:
            json.dump(self.config, json_file, indent=2, cls=ConfigJsonEncoder)

        # big data blobs are put into a separate folder
        if not os.path.exists(os.path.join(self.data_directory, "blobs")):
            os.makedirs(os.path.join(self.data_directory, "blobs"))

        for key in big:
            element = big[key]
            from branchedflowsim.io import ResultFile
            save_file = os.path.join(self.data_directory, key)
            if not os.path.exists(os.path.dirname(save_file)):
                os.makedirs(os.path.dirname(save_file))

            if isinstance(element, ResultFile):
                element.to_file(save_file)
                continue

            import numpy as np
            if isinstance(element, np.ndarray):
                np.save(save_file, element)
                continue

            if isinstance(element, PlotFunction):
                import matplotlib.pyplot as plt
                save_file = os.path.join(self.data_directory, key + "." + element.format)

                if not os.path.exists(os.path.dirname(save_file)):
                    os.makedirs(os.path.dirname(save_file))

                fig = plt.figure(figsize=(20, 20), dpi=300)
                element()
                plt.savefig(save_file)
                plt.close(fig)
                continue

            raise AssertionError("This code should be unreachable")

    def _split_data(self, data, root="."):
        from branchedflowsim.io.result_file import ResultFile
        from numbers import Number

        # split the dict in two parts: big data blobs and small metadata
        meta = {}
        big = {}
        for key in data:
            element = data[key]
            if isinstance(element, ResultFile):
                path = os.path.join(root, key + ".dat")
                big[path] = element
                meta[key] = {"__blob__": path, "__type__": "ResultFile"}
            elif isinstance(element, np.ndarray):
                path = os.path.join(root, key + ".npy")
                big[path] = element
                meta[key] = {"__blob__": path, "__type__": "numpy"}
            elif isinstance(element, PlotFunction):
                path = os.path.join(root, key)
                big[path] = element
                meta[key] = {"__type__": "Plot"}
            elif isinstance(element, (str, Number)):
                meta[key] = element
            elif isinstance(element, dict):
                mm, bb = self._split_data(element, os.path.join(root, key))
                meta[key] = mm
                big.update(bb)
            else:
                raise ValueError("Unsupported type %s for %s" % (type(data), key))
        return meta, big


class TraceExperiment(Experiment):
    """
    This `Experiment` performs a trace-reduce operation and persists the results.
    """
    def __init__(self, name, medium_spec, repeat, ray_count, work_dir=None, potgen_options=None, trace_options=None,
                 data_directory=None, persistence=None):
        super(TraceExperiment, self).__init__(name, data_directory, persistence=persistence)

        self.trace_options = trace_options
        self.potgen_options = potgen_options
        self.work_dir = work_dir
        self.ray_count = ray_count
        self.repeat = repeat
        self.medium_spec = medium_spec

    @property
    def config(self):
        return {
            "trace": self.trace_options,
            "potgen": self.potgen_options,
            "medium": self.medium_spec,
            "num_rays": self.ray_count,
            "repeat": self.repeat
        }

    def generate_data(self, dependencies):
        from branchedflowsim.tracer import reduce_trace_multiple
        return reduce_trace_multiple(self.medium_spec, self.repeat, self.ray_count, self.work_dir, self.potgen_options,
                                     **self.trace_options)


class Visualization(Experiment):
    def __init__(self, name, data_source, path):
        super(Visualization, self).__init__(name, dependencies={"data": data_source}, data_directory=path)
        self._plots = {}  # type: dict[str, PlotFunction]

    def generate_data(self, dependencies):
        data = dependencies["data"]
        for plot_fn in self._plots.values():
            plot_fn._set_cached_data(data)
        return self._plots

    def add_plot(self, key, plot_fn):
        self._plots[key] = PlotFunction(plot_fn)

    def visualize(self):
        # trigger generation of plots. Since we need to set_cached_data for the plot_fn
        # even if no new data is generated, we need to regenerate the plots here.
        self._generate_or_load_data(regenerate=True)
        # and visualize them
        for plot in self._plots.values():
            if isinstance(plot, PlotFunction):
                plot.interactive()

    def export(self, path):
        data = self.data
        if not os.path.exists(path):
            os.makedirs(path)

        for key, plot in data.items():
            if not isinstance(plot, PlotFunction):
                continue

            import matplotlib.pyplot as plt
            save_file = os.path.join(path, self.name+"-"+key + "." + plot.format)
            fig = plt.figure(figsize=(20, 20), dpi=300)
            plot()
            plt.savefig(save_file)
            plt.close(fig)

    @property
    def config(self):
        return {}
