#pragma once

namespace rapidjson
{
    namespace extend
    {
        // StringStream
        // Support Constructs StringStream with the first count characters of the character array starting with the element pointed by src
        //! Read-only string stream.
        /*! \note implements Stream concept
        */
        template <typename Encoding>
        struct GenericStringStream {
            typedef typename Encoding::Ch Ch;

            GenericStringStream(const Ch *src, const size_t count) : src_(src), head_(src), count_(count){}

            Ch Peek() const { return Tell()< count_?*src_:'\0'; }
            Ch Take() { return *src_++; }
            size_t Tell() const { return static_cast<size_t>(src_ - head_); }

            Ch* PutBegin() { RAPIDJSON_ASSERT(false); return 0; }
            void Put(Ch) { RAPIDJSON_ASSERT(false); }
            void Flush() { RAPIDJSON_ASSERT(false); }
            size_t PutEnd(Ch*) { RAPIDJSON_ASSERT(false); return 0; }

            const Ch* src_;     //!< Current read position.
            const Ch* head_;    //!< Original head of the string.
            size_t count_;    //!< Original head of the string.
        };
        //! String stream with UTF8 encoding.
        typedef GenericStringStream<UTF8<> > StringStream;
    }
    template <typename Encoding>
    struct StreamTraits<extend::GenericStringStream<Encoding> > {
        enum { copyOptimization = 1 };
    };
}
