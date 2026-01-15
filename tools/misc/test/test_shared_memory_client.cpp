#pragma GCC diagnostic warning "-Wuninitialized"

//#include <sys/mman.h>

#include <iostream>

#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>

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
    void add_a_vector(int_allocator & alloc_inst) {
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

int main() {
    //bi::shared_memory_object::remove("MySharedMemory");                      // erase any previous shared memory with same name
    //bi::remove_shared_memory_on_destroy remove_on_destroy("MySharedMemory"); // this calls remove in its destructor
    {
        //Create shared memory
        bi::managed_shared_memory segment(bi::open_read_only, "MySharedMemory"); // could use open_only instead

        //An allocator convertible to any allocator<T, segment_manager_t> type
        //void_allocator alloc_inst(segment.get_segment_manager());

        //Find the map using the c-string name

        complex_map_type *mymap = segment.find<complex_map_type>("MyMap").first;

        if (!mymap) {
            std::cout << "Odd... I didn't find MyMap in the shared segment.\n";
            return 0;
        }
        for (const auto & [mappedkey, mappedval] : (*mymap)) {
            std::cout << "Mapped key: " << mappedkey << "\t" << mappedval.show_values() << '\n';
        }

    }
    // remove is automatically called in the destructor of remove_on_destroy
    return 0;
}
