/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eoslib/types.hpp>
#include <eoslib/system.h>
#include <eoslib/memory.hpp>
#include <eoslib/print.hpp>

inline void* operator new(unsigned int size, void* __p) { return __p; }

namespace eosio {

   template <typename T>
   class vector {

   private:
      uint32_t  capacity;
      uint32_t  size;
      T*        data;
      uint32_t* ref_count;

      static T* allocate(size_t size) {
        return static_cast<T*>(malloc(sizeof(T) * size));
      }

      static uint32_t* allocate_ref() {
        uint32_t* ref = static_cast<uint32_t*>(malloc(sizeof(uint32_t)));
        *ref = 0;
        return ref;
      }

      static void copy_range(T* begin, T* end, T* dest) {
        while(begin != end) {
            new((void*)dest) T(*begin);
            ++begin;
            ++dest;
        }
      }

      static void delete_range(T* begin, T* end) {
        while(begin != end) {
            begin->~T();
            ++begin;
        }
      }

      void add_ref() {
         if( ref_count == nullptr ) return;
         (*ref_count)++;
      }

      void release() {
         if( ref_count == nullptr ) return;
         (*ref_count)--;

         if(*ref_count == 0) {
            delete_range(data, data + size);
            free(data); data=nullptr;
            free(ref_count); ref_count=nullptr;
         }
      }

      void increase_capacity( uint32_t new_capacity ) {
         T* new_data = allocate(new_capacity);
         uint32_t* new_ref_count = allocate_ref();

         copy_range(data, data + size, new_data);

         release();

         capacity  = new_capacity;
         data      = new_data;
         ref_count = new_ref_count;

         add_ref();
      }

   public:
      typedef T* iterator;
      typedef T value_type;

      vector() :  capacity(0), size(0), data(nullptr), ref_count(nullptr) { }

      vector(const T* new_data, uint32_t new_size) {
         data      = const_cast<T*>(new_data);
         size      = new_size;
         capacity  = new_size;
         ref_count = nullptr;
      }

      vector(uint32_t capacity) : capacity(capacity), size(0) {
         data      = allocate(capacity);
         ref_count = allocate_ref();
         add_ref();
      }

      vector(const vector<T>& rhs) {
         if (this != &rhs) {
            capacity   = rhs.capacity;
            size       = rhs.size;
            data       = rhs.data;
            ref_count  = rhs.ref_count;
            add_ref();
         }
      }

      vector<T>& operator=(const vector<T>& rhs) {
         if (this != &rhs) {
            release();
            capacity   = rhs.capacity;
            size       = rhs.size;
            data       = rhs.data;
            ref_count  = rhs.ref_count;
            add_ref();
         }
         return *this;
      }

      ~vector() {
         release();
      }

      void reserve(uint32_t new_capacity) {
         if(capacity >= new_capacity) return;
         increase_capacity(new_capacity);
      }

      void clear() {
         release();
         capacity  = 0;
         size      = 0;
         data      = nullptr;
         ref_count = nullptr;
      }

      uint32_t get_size() const {
         return size;
      }

      uint32_t get_capacity() const {
         return capacity;
      }

      T* get_data() const {
         return data;
      }

      uint32_t get_ref_count() {
         uint32_t tmp = 0;
         if(ref_count != nullptr) tmp = *ref_count;
         return tmp;
      }

      void push_back(const T& value) {
        if( capacity <= size ) {
            size_t new_capacity = capacity ? capacity * 2 : 10;
            increase_capacity(new_capacity);
         }

         // eosio::print("voy meto => ", value);
         new((void*)(data + size)) T(value);
         //eosio::print("-----------------------------------\n");
         ++size;

         // eosio::print(">>.imprimo\n");
         // (*this)[size].print();
         // eosio::print(">>--------\n");
      }

      T& operator[](uint32_t i) {
         //eosio::print("1voy a ver ", i, " ", size, "\n");
         assert(i >= 0 && i < size, "range error");
         return data[i];
      }

      const T& operator[](uint32_t i) const {
         //eosio::print("2voy a ver ", i, " ", size, "\n");
         assert(i >= 0 && i < size, "range error");
         return data[i];
      }

      T* begin() const {
        return data;
      }

      T* end() const {
        return data + size;
      }

   };
}