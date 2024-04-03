"""
Module for discovery and initialization of BuildTargets
"""

import app_sw_modules
import thirdparty_sw_modules
import vector_sw_modules
from vector_sw_modules.adaptive_microsar_stack import AdaptiveMicrosarStack

from core.build_target import BuildTarget
from core.virtual_build_target import VirtualBuildTarget
import utils.inspect_util as InspectUtil


# name of the VirtualBuildTarget class representing the complete stack
AMSR_STACK_NAME = 'AdaptiveMicrosarStack'

class ModuleIndex:
    """ Helper class to dynamically discover available BuildTarget classes and to initialize them """

    def __init__(self):
        # Variable storing <class_name, class_instance> key-value pairs
        # for all BuildTarget classes belonging to the stack (NOT the VirtualBuildTarget classes).
        self._class_index = dict()  # str -> to class objects

        # Variable storing <class_name, class_instance> key-value pairs
        # for all VirtualBuildTarget classes (NOT the stack itself)
        self._virtual_class_index = dict()  # str -> to class objects

        self._build_class_indexes()

    def _build_class_indexes(self):
        """ Builds the class indexes accordingly """
        buildtarget_classes = self.get_nonvirtual_buildtargets()
        for class_obj in buildtarget_classes:
            self._class_index[class_obj.get_formatted_classname()] = class_obj

        virtual_buildtarget_classes = self.get_virtual_buildtargets()
        for class_obj in virtual_buildtarget_classes:
            # do not include the representation of AMSR stack since it needs special configuration
            class_name = class_obj.get_formatted_classname()
            if class_name != AMSR_STACK_NAME:
                self._virtual_class_index[class_name] = class_obj

    def get_module(self, module_name=AMSR_STACK_NAME):
        """
        Returns the module instance if it is found in the index, else returns None.
        If the module is not already initialized, initializes it and its dependencies.
        Default value returns the virtual class representing all available modules.
        """
        module = None
        if module_name == AMSR_STACK_NAME:
            # stack requires discovered classes for its constructor
            discovered_classes_list = self._class_index.values()
            module = AdaptiveMicrosarStack(discovered_classes_list)
        elif module_name in self._class_index:
            module = self._class_index[module_name]()
        elif module_name in self._virtual_class_index:
            module = self._virtual_class_index[module_name]()

        return module

    @staticmethod
    def get_nonvirtual_buildtargets():
        """ Helper to get the list of non virtual BuildTarget classes that are available in the source code """
        InspectUtil.import_submodules(app_sw_modules)
        InspectUtil.import_submodules(thirdparty_sw_modules)
        InspectUtil.import_submodules(vector_sw_modules)

        nonvirtual_buildtarget_classes = InspectUtil.get_imported_subclasses_of_type(
            BuildTarget, exclude_types=[VirtualBuildTarget])
        return sorted(nonvirtual_buildtarget_classes, key=lambda cls_obj: cls_obj.get_formatted_classname().lower())

    @staticmethod
    def get_virtual_buildtargets():
        """ Helper to get the list of VirtualBuildTarget classes that are available in the source code """
        InspectUtil.import_submodules(app_sw_modules)
        InspectUtil.import_submodules(thirdparty_sw_modules)
        InspectUtil.import_submodules(vector_sw_modules)

        virtual_buildtarget_classes = InspectUtil.get_imported_subclasses_of_type(VirtualBuildTarget)
        return sorted(virtual_buildtarget_classes, key=lambda cls_obj: cls_obj.get_formatted_classname().lower())
