#pragma once

#include <DataTypes/DataTypeAggregateFunction.h>
#include <AggregateFunctions/IAggregateFunction.h>
#include <Common/typeid_cast.h>
#include <Common/assert_cast.h>


namespace DB
{
struct Settings;

namespace ErrorCodes
{
    extern const int ILLEGAL_TYPE_OF_ARGUMENT;
}


/** Not an aggregate function, but an adapter of aggregate functions,
  * Aggregate functions with the `Merge` suffix accept `DataTypeAggregateFunction` as an argument
  * (state of the aggregate function obtained earlier using the aggregate function with the `State` suffix)
  * and combine them with aggregation.
  */

class AggregateFunctionMerge final : public IAggregateFunctionHelper<AggregateFunctionMerge>
{
private:
    AggregateFunctionPtr nested_func;

public:
    AggregateFunctionMerge(const AggregateFunctionPtr & nested_, const DataTypePtr & argument, const Array & params_)
        : IAggregateFunctionHelper<AggregateFunctionMerge>({argument}, params_, createResultType(nested_))
        , nested_func(nested_)
    {
        const DataTypeAggregateFunction * data_type = typeid_cast<const DataTypeAggregateFunction *>(argument.get());

        if (!data_type || !nested_func->haveSameStateRepresentation(*data_type->getFunction()))
            throw Exception(ErrorCodes::ILLEGAL_TYPE_OF_ARGUMENT, "Illegal type {} of argument for aggregate function {}, "
                            "expected {} or equivalent type", argument->getName(), getName(), getStateType()->getName());
    }

    String getName() const override
    {
        return nested_func->getName() + "Merge";
    }

    static DataTypePtr createResultType(const AggregateFunctionPtr & nested_)
    {
        return nested_->getResultType();
    }

    const IAggregateFunction & getBaseAggregateFunctionWithSameStateRepresentation() const override
    {
        return nested_func->getBaseAggregateFunctionWithSameStateRepresentation();
    }

    bool isVersioned() const override
    {
        return nested_func->isVersioned();
    }

    size_t getDefaultVersion() const override
    {
        return nested_func->getDefaultVersion();
    }

    DataTypePtr getStateType() const override
    {
        return nested_func->getStateType();
    }

    void create(AggregateDataPtr __restrict place) const override
    {
        nested_func->create(place);
    }

    void destroy(AggregateDataPtr __restrict place) const noexcept override
    {
        nested_func->destroy(place);
    }

    void destroyUpToState(AggregateDataPtr __restrict place) const noexcept override
    {
        nested_func->destroyUpToState(place);
    }

    bool hasTrivialDestructor() const override
    {
        return nested_func->hasTrivialDestructor();
    }

    size_t sizeOfData() const override
    {
        return nested_func->sizeOfData();
    }

    size_t alignOfData() const override
    {
        return nested_func->alignOfData();
    }

    void add(AggregateDataPtr __restrict place, const IColumn ** columns, size_t row_num, Arena * arena) const override;

    void merge(AggregateDataPtr __restrict place, ConstAggregateDataPtr rhs, Arena * arena) const override
    {
        nested_func->merge(place, rhs, arena);
    }

    bool isAbleToParallelizeMerge() const override { return nested_func->isAbleToParallelizeMerge(); }
    bool canOptimizeEqualKeysRanges() const override { return nested_func->canOptimizeEqualKeysRanges(); }

    void parallelizeMergePrepare(AggregateDataPtrs & places, ThreadPool & thread_pool, std::atomic<bool> & is_cancelled) const override
    {
        nested_func->parallelizeMergePrepare(places, thread_pool, is_cancelled);
    }

    void merge(AggregateDataPtr __restrict place, ConstAggregateDataPtr rhs, ThreadPool & thread_pool, std::atomic<bool> & is_cancelled, Arena * arena) const override
    {
        nested_func->merge(place, rhs, thread_pool, is_cancelled, arena);
    }

    void serialize(ConstAggregateDataPtr __restrict place, WriteBuffer & buf, std::optional<size_t> version) const override
    {
        nested_func->serialize(place, buf, version);
    }

    void deserialize(AggregateDataPtr __restrict place, ReadBuffer & buf, std::optional<size_t> version, Arena * arena) const override
    {
        nested_func->deserialize(place, buf, version, arena);
    }

    void insertResultInto(AggregateDataPtr __restrict place, IColumn & to, Arena * arena) const override
    {
        nested_func->insertResultInto(place, to, arena);
    }

    bool allocatesMemoryInArena() const override
    {
        return nested_func->allocatesMemoryInArena();
    }

    AggregateFunctionPtr getNestedFunction() const override { return nested_func; }

    bool isState() const override
    {
        return nested_func->isState();
    }
};

}
