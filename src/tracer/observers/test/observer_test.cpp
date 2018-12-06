//
// Created by erik on 1/19/18.
//

#include "observers/observer.hpp"
#include <boost/test/unit_test.hpp>
#include "test_helpers.hpp"

// additional tests:
//	check master observer
//  check observer integration with tracer

BOOST_AUTO_TEST_SUITE(observer_test)

    class TestObserver : public Observer
    {
    public:
        explicit TestObserver(const std::string& file_name) : Observer(file_name) {}
        bool watch(const State&, double) override { return false; }
        void startTrajectory(const InitialCondition&, std::size_t) override {}
        void save( std::ostream& ) override { };
        std::shared_ptr<Observer> makeThreadCopy() override { return nullptr; };
    };

    /*
     * The only actual behaviour that is defined in `Observer` is the
     * caching of the desired save file name, so that is what we test
     * here.
     */
    BOOST_AUTO_TEST_CASE(observer_base_class)
    {
        TestObserver test("file_name");
        BOOST_CHECK_EQUAL(test.filename(), "file_name");

        test.setFileName("other_file");
        BOOST_CHECK_EQUAL(test.filename(), "other_file");
    }

    // -----------------------------------------------------------------------------------------------------------------

    class TestThreadLocalObserver : public ThreadLocalObserver
    {
    public:
        explicit TestThreadLocalObserver(const std::string& file_name) : ThreadLocalObserver(file_name) {}
        bool watch(const State&, double) override { return false; }
        void startTrajectory(const InitialCondition&, std::size_t) override {}
        void save( std::ostream& ) override { };

        int combined = 0;
    private:
        std::shared_ptr<ThreadLocalObserver> clone() const override {
            return std::make_shared<TestThreadLocalObserver>(filename());
        }

        void combine( ThreadLocalObserver& ) override { ++combined; }
    };

    /*
     * The ThreadLocalObserver observer adds a registration for a root observer, and reduction operations.
     * A freshly created observer is neither root nor slave.
     */
    BOOST_AUTO_TEST_CASE(thread_local_observer_ctor_dtor) {
        auto test = std::make_shared<TestThreadLocalObserver>("file_name");
        BOOST_CHECK_EQUAL(test->filename(), "file_name");
        // this is fine, test has not been assigned to another root observer, so we can delete it without error.

        BOOST_CHECK(!test->is_slave());
        BOOST_CHECK(!test->is_root());

        // unfortunately, we cannot simply test the error checking in the destructor of
        // ThreadLocalObserver, as that would immediately go to terminate and kill the test.
    }

    /*
     * When we make a new thread copy of a ThreadLocalObserver, this new observer is a slave observer,
     * and the old observer becomes a root observer. We can then perform a reduction to merge the two
     * observers, which detaches the new observer from its old root, so a second reduce will not change
     * anything.
     *
     * Unfortunately I don't know how we could test the correct use of mutexes to ensure thread safety.
     */
    BOOST_AUTO_TEST_CASE(thread_local_observer_thread_copy_and_reduce) {
        auto test = std::make_shared<TestThreadLocalObserver>("file_name");
        auto copy = std::dynamic_pointer_cast<ThreadLocalObserver>(test->makeThreadCopy());

        BOOST_CHECK(!test->is_slave());
        BOOST_CHECK(test->is_root());
        BOOST_CHECK(copy->is_slave());
        BOOST_CHECK(!copy->is_root());

        copy->reduce();
        BOOST_CHECK(!copy->is_slave());
        BOOST_CHECK_EQUAL(test->combined, 1);

        // check that reduce only works once.
        copy->reduce();
        BOOST_CHECK_EQUAL(test->combined, 1);
    }

    // -----------------------------------------------------------------------------------------------------------------

    class TestThreadSharedObserver : public ThreadSharedObserver
    {
    public:
        explicit TestThreadSharedObserver(const std::string& file_name) : ThreadSharedObserver(file_name) {}
        bool watch(const State&, double) override { return false; }
        void startTrajectory(const InitialCondition&, std::size_t) override {}
        void save( std::ostream& ) override { };
    };

    /*
     * Check that file name is correctly propagated.
     */
    BOOST_AUTO_TEST_CASE(thread_shared_observer_ctor) {
        TestThreadSharedObserver test("file_name");
        BOOST_CHECK_EQUAL(test.filename(), "file_name");
    }

    /*
     * Check that thread copy always refers to the same element.
     */
    BOOST_AUTO_TEST_CASE(thread_shared_observer_copy) {
        auto test = std::make_shared<TestThreadSharedObserver>("file_name");
        auto copy = std::dynamic_pointer_cast<TestThreadSharedObserver>(test->makeThreadCopy());

        // refer to the same object
        BOOST_CHECK(test.get() == copy.get());
        auto m1 = test->getLock().mutex();
        auto m2 = copy->getLock().mutex();
        BOOST_CHECK(m1 == m2);
    }



BOOST_AUTO_TEST_SUITE_END()