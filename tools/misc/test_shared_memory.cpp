#pragma GCC diagnostic warning "-Wuninitialized"

//#include <sys/mman.h>

#include <iostream>

#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>

//#include <boost/interprocess/managed_heap_memory.hpp> // could do memory size tests here if I wanted to create types for that

namespace bi = boost::interprocess;

//Typedefs of allocators and containers
typedef bi::managed_shared_memory::segment_manager segment_manager_t;
typedef bi::allocator<void, segment_manager_t> void_allocator;
typedef bi::allocator<int, segment_manager_t> int_allocator;
typedef bi::vector<int, int_allocator> int_vector;
typedef bi::allocator<int_vector, segment_manager_t> int_vector_allocator;
typedef bi::vector<int_vector, int_vector_allocator> int_vector_vector;
typedef bi::allocator<char, segment_manager_t> char_allocator;
typedef bi::basic_string<char, std::char_traits<char>, char_allocator> char_string;

class complex_data {
    int id_;
    char_string char_string_;
    int_vector_vector int_vector_vector_;

public:
    //Since void_allocator is convertible to any other allocator<T>, we can simplify
    //the initialization taking just one allocator for all inner containers.
    complex_data(int id, const char *name, const void_allocator &void_alloc)
        : id_(id), char_string_(name, void_alloc), int_vector_vector_(void_alloc) {}
    //Other members...
    std::string show_values() const {
        std::string s;
        std::string sstr(char_string_.begin(),char_string_.end());
        s += "ID=" + std::to_string(id_) + ", ";
        s += "string=" + sstr + ", ";
        s += "vec size=" + std::to_string(int_vector_vector_.size());
        return s;
    }
    void add_a_vector(int_allocator alloc_inst) {
        int_vector ivec(alloc_inst);
        ivec.emplace_back(1);
        int_vector_vector_.push_back(ivec); // quite the dummy, just adds an empty vector
    }
};

//Definition of the map holding a string as key and complex_data as mapped type
typedef std::pair<const char_string, complex_data> map_value_type;
typedef std::pair<char_string, complex_data> movable_to_map_value_type;
typedef bi::allocator<map_value_type, segment_manager_t> map_value_type_allocator;
typedef bi::map<char_string, complex_data, std::less<char_string>, map_value_type_allocator> complex_map_type;

void show_segment_info(bi::managed_shared_memory & segment) {
    std::cout << "  number of named objects  = " << segment.get_num_named_objects() << '\n';
    std::cout << "  number of unique objects = " << segment.get_num_unique_objects() << '\n';
    std::cout << "  size                     = " << segment.get_size() << '\n';
    //std::cout << "  real size                = " << segment.get_real_size() << '\n';
    //std::cout << "  user size                = " << segment.get_user_size() << '\n';
    std::cout << "  free memory              = " << segment.get_free_memory() << '\n';
    //bi::managed_shared_memory::get_instance_length(mymap);
    unsigned long the_result = segment.get_size() - segment.get_free_memory();
    std::cout << "  memory used              = " << the_result << '\n';
}

/// Just testing some allocation of the data type being used to see how much space it needs.
unsigned long test_sizes() {
    bi::shared_memory_object::remove("MySharedMemory");
    bi::remove_shared_memory_on_destroy remove_on_destroy("MySharedMemory");
    unsigned long the_result = 0;
    {
        bi::managed_shared_memory segment(bi::create_only, "MySharedMemory", 65536);
        void_allocator alloc_inst(segment.get_segment_manager());
        complex_map_type *mymap = segment.construct<complex_map_type>("MyMap")(std::less<char_string>(), alloc_inst);
        for (int i = 0; i < 2; ++i) {
            //Both key(string) and value(complex_data) need an allocator in their constructors
            char_string key_object(alloc_inst);
            key_object = ("somekey-"+std::to_string(i)).c_str();
            complex_data mapped_object(i, "default_name", alloc_inst);
            if (i==0) {
                mapped_object.add_a_vector(alloc_inst); // allocate just one vector in the vector
            } else {
                for (int j = 0; j < 100; ++j)
                    mapped_object.add_a_vector(alloc_inst); // allocate a hundred vectors
            }
            map_value_type value(key_object, mapped_object);
            //Modify values and insert them in the map
            mymap->insert(value);
        }
        // now let's find out how big this lopsided complex data structure is
        std::cout << "Size test (after allocting 2 vectors in the complex map, with 1 and 100 entries):\n";
        show_segment_info(segment);
        the_result = segment.get_size() - segment.get_free_memory();
    }
    return the_result;
}

int main() {
    unsigned long thesize = test_sizes();
    thesize *= 100;
    std::cout << "Allocating a complex object with 100 key-value objects containing\nvector of vectors.\n";
    std::cout << "Using a shared memory segment of " << thesize << " bytes\n";
    bi::shared_memory_object::remove("MySharedMemory");                      // erase any previous shared memory with same name
    bi::remove_shared_memory_on_destroy remove_on_destroy("MySharedMemory"); // this calls remove in its destructor
    { // don't need a try here, because remove is automatic and we'd throw it again anyway
        //Create shared memory
        bi::managed_shared_memory segment(bi::create_only, "MySharedMemory", thesize); //65536);

        /*
        NOTE: Instead of estimating the size the allocate as well as possible, it is also  possible to
        use shrink_to_fit() as per
        https://valelab4.ucsf.edu/svn/3rdpartypublic/boost/doc/html/interprocess/managed_memory_segments.html,
        but beware that the server should then first use create_only, allocate data, then exit the scope in
        which that segment was defined, so that it isn't actively being managed by a process, then shrink
        to fit, and then open again to find the allocated data. (All of this within the active server
        process, i.e. before exiting and removing the shared memory.)
        See the example under "Growing managed segments" in the Boost documentation.
        */

        //An allocator convertible to any allocator<T, segment_manager_t> type
        void_allocator alloc_inst(segment.get_segment_manager());

        //Construct the shared memory map and fill it
        complex_map_type *mymap = segment.construct<complex_map_type>
                                  //(object name), (first ctor parameter, second ctor parameter)
                                  ("MyMap")(std::less<char_string>(), alloc_inst);

        for (int i = 0; i < 100; ++i) {
            //Both key(string) and value(complex_data) need an allocator in their constructors
            char_string key_object(alloc_inst);
            key_object = ("somekey-"+std::to_string(i)).c_str();
            complex_data mapped_object(i, "default_name", alloc_inst);
            for (int j = 0; j < i; ++j)
                mapped_object.add_a_vector(alloc_inst);
            map_value_type value(key_object, mapped_object);
            //Modify values and insert them in the map
            mymap->insert(value);
        }
        show_segment_info(segment);
        std::cout << "\nCreated MysharedMemory and MyMap. Staying resident until you press ENTER...\n";
        std::string enterstr;
        std::getline(std::cin, enterstr);
    }
    // remove is automatically called in the destructor of remove_on_destroy
    return 0;
}
