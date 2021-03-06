#ifndef FPMAS_CSV_OUTPUT_H
#define FPMAS_CSV_OUTPUT_H

#include <fstream>
#include <functional>
#include <vector>
#include <sstream>
#include "output.h"
#include "fpmas/communication/communication.h"

/** \file fpmas/io/csv_output.h
 *
 * CSV based implementation of fpmas::api::output.
 */

namespace fpmas { namespace io {
	using api::io::Watcher;


	/**
	 * Component used to convert an arbitrary data field to an std::string in
	 * the CSV serialization process.
	 *
	 * By default, serializes `data` using the C++ output stream operator, but
	 * template specifilizations can be provided to define other behaviors for
	 * specific types.
	 *
	 * @tparam T type of the field to serialize
	 */
	template<typename T>
		struct CsvSerial {
			/**
			 * Converts `data` to an `std::string` using the C++ output stream
			 * operato `<<`.
			 *
			 * @param data data to serialize
			 * @return string representation of `data`
			 */
			static std::string to_csv(const T& data) {
				std::ostringstream stream;
				stream << data;
				return stream.str();
			}
		};
	
	/**
	 * CSV based api::output::Output implementation.
	 *
	 * Produces very simple CSV formatted data, without any escaping.
	 *
	 * Each field is serialized using the C++ stream output operator `<<`
	 * applied to each `DataField`.
	 *
	 * @param DataField types of data in each row
	 * 
	 * This is a partial implementation that cannot be used directly.
	 * @see CsvOutput
	 * @see DistributedCsvOutput
	 */
	template<typename... DataField>
	class CsvOutputBase : public OutputBase {
		private:
			std::vector<std::string> _headers;
			std::tuple<std::function<DataField()>...> watchers;
	
			/*
			 * Tuple unrolling helper structs.
			 */
			template<int ...> struct seq {};
			template<int N, int ...S> struct gens {
				public:
					typedef typename gens<N-1, N-1, S...>::type type;
			}; template<int ...S> struct gens<0, S...>{ typedef seq<S...> type; };

			/*
			 * Unrolls the input tuple to feed internal _headers, that are then
			 * accessible with headers().
			 */
			template<int ...S>
				void build_headers(
						std::tuple<std::pair<std::string, std::function<DataField()>>...> csv_fields,
						const seq<S...>) {
					_headers = {std::get<S>(csv_fields).first...};
				}

			/*
			 * Unrolls the input tuple and calls each watcher. 
			 * Data is serialized and returned in a vector.
			 */
			template<int ...S>
				std::vector<std::string> _dump_fields(const seq<S...>) {
					std::tuple<DataField...> data {std::get<S>(watchers)() ...};
					std::ostringstream output_str;
					return {CsvSerial<typename std::tuple_element<S, decltype(data)>::type>::to_csv(std::get<S>(data))...};
				}

		protected:
			/**
			 * Writes `data` to the `output_stream` as CSV, considering each
			 * item in `data` as a CSV field.
			 *
			 * @param data data to write
			 */
			void dump_csv(const std::vector<std::string>& data) {
				std::ostream& output = output_stream.get();
				for(std::size_t i = 0; i < data.size()-1; i++)
					output << data[i] << ",";
				output << data.back() << std::endl;
			}

			/**
			 * Returns the CSV headers used by this output.
			 *
			 * @return CSV headers
			 */
			std::vector<std::string> headers() {
				return _headers;
			}
			/**
			 * Fetch data fields using the `watchers` and returns the
			 * serialized data.
			 *
			 * @return serialized data
			 */
			std::vector<std::string> dump_fields() {
				return _dump_fields(typename gens<sizeof...(DataField)>::type());
			}


			/**
			 * CsvOutputBase constructor.
			 *
			 * @param output_stream stream to which data will be dumped
			 * @param csv_fields pairs of type `{"field_name", watcher}`, where
			 * `watcher` is a callable object used to fetch data corresponding
			 * to `"field_name"`
			 */
			CsvOutputBase(
					api::io::OutputStream& output_stream,
					std::pair<std::string, std::function<DataField()>>... csv_fields)
				: OutputBase(output_stream), watchers(csv_fields.second...) {
					build_headers({csv_fields...}, typename gens<sizeof...(DataField)>::type());
				}
	};

	/**
	 * Basic api::output::Output implementation that dumps CSV data to an
	 * output stream.
	 *
	 * @param DataField types of data in each row
	 */
	template<typename... DataField>
		class CsvOutput : public CsvOutputBase<DataField...> {
			public:
				/**
				 * CsvOutputBase constructor.
				 *
				 * Writes CSV headers to the `output_stream`.
				 *
				 * @param output_stream stream to which data will be dumped
				 * @param csv_fields pairs of type `{"field_name", watcher}`,
				 * where `watcher` is a callable object used to fetch data
				 * corresponding to `"field_name"`. See examples for concrete
				 * use cases.
				 */
				CsvOutput(
					api::io::OutputStream& output_stream,
					std::pair<std::string, std::function<DataField()>>... csv_fields)
					: CsvOutputBase<DataField...>(output_stream, csv_fields...) {
						this->dump_csv(this->headers());
					}

				/**
				 * Dumps data fields to the `output_stream`.
				 *
				 * Each data field is dynamically fetched using the
				 * corresponding `watcher` provided in the constructor.
				 */
				void dump() override {
					this->dump_csv(this->dump_fields());
				};
		};

	/**
	 * Type used as a distributed CSV field parameter.
	 *
	 * This allows to optionally pass a custom set of paramaters to the
	 * `DistributedOperation` when building a DistributedCsvOutput.
	 */
	template<typename Operation>
		struct DistributedCsvField : public std::tuple<
									std::string,
									Watcher<typename Operation::Type>,
									typename Operation::Params>
	{
		/**
		 * Imports standard tuple constructors.
		 */
		using std::tuple<
			std::string,
		std::function<typename Operation::Type()>,
		typename Operation::Params>::tuple;

		/**
		 * Custom DistributedCsvField constructor.
		 *
		 * The `Params` field of the tuple is default initialized.
		 */
		DistributedCsvField(
				const std::string& name,
				std::function<typename Operation::Type()> watcher)
			: DistributedCsvField(name, watcher, {}) {}
	};

	/**
	 * A distributed api::output::Output implementation that dumps CSV data to an
	 * output stream.
	 *
	 * Data is automatically gathered on **all** processes, and can be dumped
	 * on one processes or on all the processes.
	 *
	 * @param DataField types of data in each row
	 */
	template<typename... DataFieldOperation>
		class DistributedCsvOutput : public CsvOutputBase<typename DataFieldOperation::Type...> {
			private:
				api::communication::MpiCommunicator& comm;
				int root;
				bool all_out;

			public:
				/**
				 * DistributedCsvOutput constructor.
				 *
				 * When this version is used, specifying a `root` parameter,
				 * data is only dumped on `output_stream` on the process
				 * corresponding to `root` (according to the specified `comm`).
				 * On all other processes, `output_stream` is ignored.
				 *
				 * @param comm MPI communicator
				 * @param root rank of the process on which data is dumped
				 * @param output_stream output stream on which data are dumped
				 * (ignored on processes other than `root`)
				 * @param csv_fields tuples of type `{"field_name", watcher,
				 * params}`, where `watcher` is a callable object used to fetch
				 * data corresponding to `"field_name"`. `params` is optional.
				 * See examples for concrete use cases.
				 */
				DistributedCsvOutput(
						api::communication::MpiCommunicator& comm, int root,
						api::io::OutputStream& output_stream,
						DistributedCsvField<DataFieldOperation>... csv_fields)
					: CsvOutputBase<typename DataFieldOperation::Type...>(
							output_stream,
							{
							// Field name
							std::get<0>(csv_fields),
							// DistributedOperation
							typename DataFieldOperation::Single(
									comm, root,
									// Watcher
									std::get<1>(csv_fields),
									// Extra Params
									std::get<2>(csv_fields))
							}...),
					comm(comm), root(root), all_out(false)
					{
						if(comm.getRank() == root)
							this->dump_csv(this->headers());
					}


				/**
				 * DistributedCsvOutput constructor.
				 *
				 * When this version is used, withou specifying a `root` parameter,
				 * data is dumped on `output_stream` on all the processes.
				 *
				 * @param comm MPI communicator
				 * @param output_stream output stream on which data are dumped
				 * on each process
				 * @param csv_fields tuples of type `{"field_name", watcher,
				 * params}`, where `watcher` is a callable object used to fetch
				 * data corresponding to `"field_name"`. `params` is optional.
				 * See examples for concrete use cases.
				 */
				DistributedCsvOutput(
						api::communication::MpiCommunicator& comm,
						api::io::OutputStream& output_stream,
						DistributedCsvField<DataFieldOperation>... csv_fields)
					: CsvOutputBase<typename DataFieldOperation::Type...>(
							output_stream,
							{
							// Field name
							std::get<0>(csv_fields),
							// DistributedOperation
							typename DataFieldOperation::All(
									comm,
									// Watcher
									std::get<1>(csv_fields),
									// Extra Params
									std::get<2>(csv_fields))
							}...),
					comm(comm), all_out(true)
					{
						this->dump_csv(this->headers());
					}

				/**
				 * The dump process is implictly performed in several steps:
				 * 1. Fetch local data on each process
				 * 2. Gather data
				 * 3. Reduces gathered data
				 * 4. Serialize and dump CSV formatted data to
				 * `output_stream`
				 *
				 * Depending on the constructor used, data is gathered and
				 * dumped only at `root` or on all the processes.
				 */
				void dump() override {
					std::vector<std::string> data_fields = this->dump_fields();
					if(all_out || comm.getRank() == root)
						this->dump_csv(data_fields);
				}
		};

}}
#endif
