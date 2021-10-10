#pragma once

#include "bcos-framework/interfaces/storage/StorageInterface.h"
#include "bcos-framework/libstorage/StateStorage.h"

namespace bcos::test
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
class MockTransactionalStorage : public bcos::storage::TransactionalStorageInterface
{
    void asyncGetPrimaryKeys(const std::string_view& table,
        const std::optional<storage::Condition const>& _condition,
        std::function<void(Error::UniquePtr&&, std::vector<std::string>&&)> _callback) noexcept
        override
    {
        m_storage->asyncGetPrimaryKeys(table, _condition, std::move(_callback));
    }

    void asyncGetRow(const std::string_view& table, const std::string_view& _key,
        std::function<void(Error::UniquePtr&&, std::optional<storage::Entry>&&)> _callback) noexcept
        override
    {
        m_storage->asyncGetRow(table, _key, std::move(_callback));
    }

    void asyncGetRows(const std::string_view& table,
        const std::variant<const gsl::span<std::string_view const>,
            const gsl::span<std::string const>>& _keys,
        std::function<void(Error::UniquePtr&&, std::vector<std::optional<storage::Entry>>&&)>
            _callback) noexcept override
    {
        m_storage->asyncGetRows(table, _keys, std::move(_callback));
    }

    void asyncSetRow(const std::string_view& table, const std::string_view& key,
        storage::Entry entry, std::function<void(Error::UniquePtr&&)> callback) noexcept override
    {
        m_storage->asyncSetRow(table, key, std::move(entry), std::move(callback));
    }

    void asyncPrepare(const TwoPCParams& params,
        const storage::TraverseStorageInterface::ConstPtr& storage,
        std::function<void(Error::Ptr&&, uint64_t)> callback) noexcept override
    {
        callback(nullptr, 0);
    }

    void asyncCommit(
        const TwoPCParams& params, std::function<void(Error::Ptr&&)> callback) noexcept override
    {
        callback(nullptr);
    }

    void asyncRollback(
        const TwoPCParams& params, std::function<void(Error::Ptr&&)> callback) noexcept override
    {
        callback(nullptr);
    }

    bcos::storage::StateStorage::Ptr m_storage;
};
#pragma GCC diagnostic pop
}  // namespace bcos::test