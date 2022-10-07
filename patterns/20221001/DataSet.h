/// 数据集

#pragma once

#include <string>
#include <memory>
#include "plf_hive.h"
#include "ticket_map.hpp"

namespace abc
{
	class IDataSet {
	public:
		virtual ~IDataSet() = default;
	};

	template<typename T>
	class DataSet final :public IDataSet {
	public:
		template<typename... Args>
		T* insert(Args&&... args) {
			auto it = m_datas.insert(std::forward<Args>(args)...);
			return &(*it);
		}

		void erase(T* vp) {
			m_datas.erase(m_datas.get_iterator(vp));
		}

		void erase(const T* vp) {
			m_datas.erase(m_datas.get_iterator(vp));
		}
	private:
		plf::hive<T> m_datas;
	};

	class DataSets {
	public:
		template<typename T>
		int add(const std::string& description) {
			for (auto& [ticket, value] : m_units) {
				if (value.description == description) {
					return ticket;
				}
			}
			return m_units.insert(DataSetStub{
				description,
				std::make_unique<DataSet<T>>()
				});
		}

		template<typename T>
		int add() {
			return add<T>(typeid(T).name());
		}

		template<typename T>
		DataSet<T>* find(int idx) {
			if (auto it = m_units.find(idx); it != m_units.end()) {
				return dynamic_cast<DataSet<T>*>(it->value.set.get());
			}
			return nullptr;
		}

		template<typename T>
		DataSet<T>* find() {
			for (auto& [ticket, value] : m_units) {
				if (value.description == typeid(T).name()) {
					return dynamic_cast<DataSet<T>*>(value.set.get());
				}
			}
			return nullptr;
		}
	private:
		struct DataSetStub {
			std::string description;
			std::unique_ptr<IDataSet> set;
		};
		//数据集
		jss::ticket_map<int, DataSetStub> m_units;
	};


	/// @brief 实体
	struct Entity {
		int dataset; //实体所属数据集
		int id;  //实体ID
		std::uintptr_t pointer; //实体对应的主类指针

		std::vector<std::pair<int, std::uintptr_t>> pointers;//与该实体相关的其它数据集及附加数据指针
	};

	class Project {
	public:
		DataSets datasets;

		template<typename T>
		std::uintptr_t append(T&& v) {
			return reinterpret_cast<std::uintptr_t>(
				datasets.find<T>()->insert(std::forward<T>(v))
				);
		}

		//所有实体共享一个ID生成：TODO FIXME 应该允许不同数据集中的ID重复
		jss::ticket_map<int, Entity*> entitys;
	private:

		//map的第一层是DataSet,第二层是具体的Entity
		//jss::ticket_map<int, std::unique_ptr<jss::ticket_map<int, Entity*>>> m_entitys;
	};


	void test_project();
}
