#pragma once
#include <retro/common.hpp>
#include <retro/format.hpp>
#include <retro/func.hpp>
#include <retro/rc.hpp>
#include <retro/dyn.hpp>
#include <retro/neo.hpp>
#include <retro/platform.hpp>
#include <retro/interface.hpp>
#include <retro/bind/common.hpp>
#include <memory>
#undef assert

extern "C" {
	#include <node/node_api.h>
};

namespace retro::bind::js {
	struct value;
	struct reference;
	struct prototype;
	struct array;
	struct object;
	struct function;
	struct typedecl;
	struct sync_token;

	// Define the engine type.
	//
	struct engine {
		using value_type		 = value;
		using reference_type  = reference;
		using prototype_type  = prototype;
		using array_type		 = array;
		using object_type		 = object;
		using function_type	 = function;
		using typedecl_type   = typedecl;
		using sync_token_type = sync_token;

		napi_env env;
		constexpr engine() : env(nullptr) {}
		constexpr engine(napi_env env) : env(env) {}
		constexpr operator napi_env() const noexcept { return env; }

		// Gets globals.
		//
		object globals() const;

		// If status is not napi_ok, throws an error.
		//
		inline void assert(napi_status status, size_t ln = __builtin_LINE()) const {
			if (status == napi_ok)
				return;
			const napi_extended_error_info* result;
			RC_ASSERT(napi_get_last_error_info(env, &result) == napi_ok);
			throw std::runtime_error{fmt::str("JS Error(%d@%llu): %s", status, ln, result->error_message)};
		}

		// Throws an error.
		//
		inline void throw_error(const char* msg, const char* code = nullptr) const noexcept { napi_throw_error(env, code, msg); }
		inline void throw_type_error(const char* msg, const char* code = nullptr) const noexcept { napi_throw_type_error(env, code, msg); }
		inline void throw_range_error(const char* msg, const char* code = nullptr) const noexcept { napi_throw_range_error(env, code, msg); }
		inline void throw_error(const std::string& msg, const char* code = nullptr) const noexcept { napi_throw_error(env, code, msg.c_str()); }
		inline void throw_type_error(const std::string& msg, const char* code = nullptr) const noexcept { napi_throw_type_error(env, code, msg.c_str()); }
		inline void throw_range_error(const std::string& msg, const char* code = nullptr) const noexcept { napi_throw_range_error(env, code, msg.c_str()); }

		// Exports the given user type to the given namespace.
		//
		template<typename T, typename Into>
		inline void export_type(Into&& mod) const {
			prototype proto = {*this, type_descriptor<T>::name};
			type_descriptor<T>::write(proto);
			mod.set(type_descriptor<T>::name, proto.write<T>()->ctor);
		}
	};

	// Define the value type.
	//
	struct value {
		using engine_type = engine;

		engine	  env = {};
		napi_value val = nullptr;

		// Construction.
		//
		constexpr value()										= default;
		constexpr value(value&&) noexcept				= default;
		constexpr value(const value&)						= default;
		constexpr value& operator=(value&&) noexcept = default;
		constexpr value& operator=(const value&)		= default;
		value(napi_env env, napi_value val) : env(env), val(val) {}

		// Native observers.
		//
		engine	  context() const { return env; }
		napi_value native() const { return val; }

		operator napi_env() const { return env; }
		operator napi_value() const { return val; }

		explicit	  operator bool() const { return val != nullptr; }

		// Wrapper around type converter.
		//
		template<typename T>
		bool is() const {
			return converter<engine, T>{}.is(*this);
		}
		template<typename T>
		T as(bool coerce = false) const {
			return converter<engine, T>{}.as(*this, coerce);
		}
		template<typename T>
		static value make(napi_env env, T&& value) {
			return converter<engine, std::decay_t<T>>{}.from(env, std::forward<T>(value));
		}

		// Instance check.
		//
		template<typename T>
		bool isinstance() const;

		// String conversion.
		//
		std::string to_string() const {
			if (val) {
				napi_value res;
				if (!napi_coerce_to_string(env, val, &res)) {
					return value(env, res).template as<std::string>(false);
				} else {
					return fmt::str("JSValue %p", val);
				}
			} else {
				return "null";
			}
		}

		// Type getter.
		//
		napi_valuetype type() const noexcept {
			napi_valuetype type = napi_null;
			if (val)
				napi_typeof(env, val, &type);
			return type;
		}
	};

	// Define the reference type.
	//
	struct reference {
		using engine_type = engine;

		engine	env = {};
		napi_ref ref = nullptr;

		// Construction.
		//
		constexpr reference() = default;
		reference(value v) : env(v.env) {
			if (v) {
				env.assert(napi_create_reference(env, v.val, 1, &ref));
			}
		}

		// Copy.
		//
		reference(const reference& o) : env(o.env), ref(o.ref) {
			if (ref) {
				u32 o = 0;
				env.assert(napi_reference_ref(env, ref, &o));
			}
		}
		reference& operator=(const reference& o) {
			if (o.ref) {
				u32 p = 0;
				env.assert(napi_reference_ref(o.env, o.ref, &p));
			}
			reset();
			env = o.env;
			ref = o.ref;
			return *this;
		}

		// Move.
		//
		constexpr reference(reference&& o) noexcept { swap(o); }
		constexpr reference& operator=(reference&& o) noexcept {
			swap(o);
			return *this;
		}
		constexpr void swap(reference& o) {
			std::swap(env, o.env);
			std::swap(ref, o.ref);
		}

		// Lock into local value.
		//
		value lock() const {
			value v{env, nullptr};
			if (ref) {
				env.assert(napi_get_reference_value(env, ref, &v.val));
			}
			return v;
		}

		// Native observers.
		//
		engine	context() const { return env; }
		napi_ref native() const { return ref; }

		operator napi_env() const { return env; }
		operator napi_ref() const { return ref; }

		explicit operator bool() const { return ref != nullptr; }

		// Deref on destruction.
		//
		void reset() {
			u32 r = 0;
			if (auto prev = std::exchange(ref, nullptr)) {
				env.assert(napi_reference_unref(env, prev, &r));
				if (!r) {
					env.assert(napi_delete_reference(env, prev));
				}
			}
		}
		~reference() { reset(); }
	};

	// Define the array type.
	//
	struct array : value {
		array()									  = default;
		array(array&&) noexcept				  = default;
		array(const array&)					  = default;
		array& operator=(array&&) noexcept = default;
		array& operator=(const array&)	  = default;
		array(value v) : value(v) {}
		array(napi_env env, napi_value val) : value(env, val) {}
		array& operator=(value v) {
			value::val = v.val;
			value::env = v.env;
			return *this;
		};

		// Creates an array.
		//
		static array make(engine env, size_t length = 0) {
			array arr{env, nullptr};
			if (length)
				env.assert(napi_create_array_with_length(env, length, &arr.val));
			else
				env.assert(napi_create_array(env, &arr.val));
			return arr;
		}

		// Element operations.
		//
		value get(size_t idx) const {
			value result{value::env, nullptr};
			napi_get_element(value::env, value::val, narrow_cast<u32>(idx), &result.val);
			return result;
		}
		template<typename T>
		void set(size_t idx, const T& value) const {
			value::env.assert(napi_set_element(value::env, value::val, narrow_cast<u32>(idx), value::make(value::env, value)));
		}
		bool has(size_t idx) const {
			bool result = false;
			napi_has_element(value::env, value::val, narrow_cast<u32>(idx), &result);
			return result;
		}
		template<typename T>
		void push(const T& value) const {
			set(length(), value);
		}

		// Length getter.
		//
		size_t length() const {
			u32 len = 0;
			if (value::val)
				napi_get_array_length(value::env, value::val, &len);
			return len;
		}
	};

	// Define the object type.
	//
	struct object : value {
		object()										  = default;
		object(object&&) noexcept				  = default;
		object(const object&)					  = default;
		object& operator=(object&&) noexcept = default;
		object& operator=(const object&)	  = default;
		object(value v) : value(v) {}
		object(napi_env env, napi_value val) : value(env, val) {}
		object& operator=(value v) {
			value::val = v.val;
			value::env = v.env;
			return *this;
		};

		// Creates an object.
		//
		static object make(engine env, size_t = 0) {
			object obj{env, nullptr};
			env.assert(napi_create_object(env, &obj.val));
			return obj;
		}

		// Property operations.
		//
		value get(const char* idx) const {
			value result{value::env, nullptr};
			value::env.assert(napi_get_named_property(value::env, value::val, idx, &result.val));
			return result;
		}
		template<typename T>
		void set(const char* idx, const T& value) const {
			value::env.assert(napi_set_named_property(value::env, value::val, idx, value::make(value::env, value)));
		}
		bool has(const char* idx) const {
			bool result = false;
			value::env.assert(napi_has_named_property(value::env, value::val, idx, &result));
			return result;
		}
		value get(value idx) const {
			value result{value::env, nullptr};
			value::env.assert(napi_get_property(value::env, value::val, idx, &result.val));
			return result;
		}
		template<typename T>
		void set(value idx, const T& value) const {
			value::env.assert(napi_set_property(value::env, value::val, idx, value::make(value::env, value)));
		}
		bool has(value idx) const {
			bool result = false;
			value::env.assert(napi_has_property(value::env, value::val, idx, &result));
			return result;
		}

		template<typename F>
		void for_each(F&& fn) const {
			array arr{env, nullptr};
			env.assert(napi_get_property_names(env, val, &arr.val));

			uint32_t len = arr.length();
			for (uint32_t i = 0; i != len; i++) {
				fn(arr.get(i), get(arr.get(i)));
			}
		}

		// Extensions.
		//
		void seal() const { value::env.assert(napi_object_seal(value::env, value::val)); }
		void freeze() const { value::env.assert(napi_object_freeze(value::env, value::val)); }
	};
	inline object engine::globals() const {
		object val = {env, nullptr};
		this->assert(napi_get_global(env, &val.val));
		return val;
	}

	// Async work interface.
	//
	struct async_work {
		napi_async_work work;

		// Virtual functors.
		//
		virtual void execute(napi_env env)							= 0;
		virtual void complete(napi_env env, napi_status status) = 0;

		// Constructed by an environment and optionally a resource.
		//
		async_work(engine env, napi_value resource = {}) {
			napi_value resource_name;
			env.assert(napi_create_string_utf8(env, "", 0, &resource_name));

			napi_create_async_work(
				 env, resource, resource_name,
				 [](napi_env env, void* data) {
					 auto* self = (async_work*) data;
					 self->execute(env);
				 },
				 [](napi_env env, napi_status status, void* data) {
					 std::unique_ptr<async_work> self{(async_work*) data};
					 self->complete(env, status);
					 self->destroy(env);
				 },
				 this, &work);
		}

		// Work commands.
		//
		void queue(engine env) { env.assert(napi_queue_async_work(env, work)); }
		void cancel(engine env) { env.assert(napi_cancel_async_work(env, work)); }
		void destroy(engine env) { env.assert(napi_delete_async_work(env, work)); }

		// Virtual destructor.
		//
		virtual ~async_work() = default;
	};

	// Function conversion.
	//
	namespace detail {
		template<typename Tup>
		struct tuple_decay;

		template<typename... Tx>
		struct tuple_decay<std::tuple<Tx...>> {
			using type = std::tuple<std::decay_t<Tx>...>;
		};

		template<size_t N, typename Tup>
		inline void pack_tuple(Tup& out, napi_env env, napi_value* arr) {
			if constexpr (N < std::tuple_size_v<Tup>) {
				using T			  = typename std::tuple_element_t<N, Tup>;
				if constexpr (std::is_same_v<engine, T>) {
					std::get<N>(out) = env;
					return pack_tuple<N + 1, Tup>(out, env, arr - 1);
				} else {
					std::get<N>(out) = value{env, arr[N]}.template as<T>(true);
					return pack_tuple<N + 1, Tup>(out, env, arr);
				}
			}
		}
		template<size_t N, typename Tup>
		inline void unpack_tuple(const Tup& in, napi_env env, napi_value* arr) {
			if constexpr (N < std::tuple_size_v<Tup>) {
				arr[N] = value::make(env, std::get<N>(in));
				return unpack_tuple<N + 1, Tup>(in, env, arr);
			}
		}

		template<typename Fn, typename R, typename Args>
		struct async_task final : async_work {
			using Rs = std::conditional_t<std::is_void_v<R>, std::monostate, R>;

			Fn											functor;
			Args										args;
			napi_deferred							deferred;
			std::variant<Rs, std::exception> result = {};

			async_task(napi_env env, Fn functor, Args args, napi_deferred deferred, napi_value resource = {})
				 : async_work(env, resource), functor(std::move(functor)), args(std::move(args)), deferred(deferred) {}

			void execute(napi_env) override {
				try {
					if constexpr (std::is_void_v<R>) {
						std::apply(functor, std::move(args));
						result.template emplace<0>(std::monostate{});
					} else {
						result.template emplace<0>(std::apply(functor, std::move(args)));
					}
				} catch (std::exception ex) {
					result.template emplace<1>(ex);
				}
			}
			void complete(napi_env _env, napi_status status) override {
				engine env{_env};
				if (status != napi_ok) {
					return env.assert(napi_reject_deferred(env, deferred, value::make<std::string_view>(env, "Task cancelled")));
				}
				try {
					if (auto* val = std::get_if<0>(&result))
						env.assert(napi_resolve_deferred(env, deferred, value::make(env, std::move(*val))));
					else
						env.assert(napi_reject_deferred(env, deferred, value::make<std::string_view>(env, std::get<1>(result).what())));
				} catch (std::exception ex) {
					env.assert(napi_reject_deferred(env, deferred, value::make<std::string_view>(env, ex.what())));
				}
			}
		};

		template<typename R, typename Tuple, typename F>
		inline value apply_async(engine env, F&& fn, Tuple&& tuple) {
			// Create the deferred and promise types.
			//
			napi_deferred def;
			napi_value	  promise;
			env.assert(napi_create_promise(env, &def, &promise));

			// Create the task and queue it.
			//
			auto task = std::make_unique<detail::async_task<std::decay_t<F>, R, Tuple>>(env, std::forward<F>(fn), std::forward<Tuple>(tuple), def);
			task->queue(env);
			task.release();

			// Return the promise.
			//
			return {env, promise};
		}
	};
	
	struct callback_details {
		napi_callback callback;
		napi_finalize finalizer;
		void*			  data;
	};
	template<typename F, bool HasThis, bool Async>
	struct callback_builder {
		// Resolve function traits.
		//
		using traits = function_traits<F>;
		using Tuple	 = typename detail::tuple_decay<typename traits::arguments>::type;
		using R		 = typename traits::return_type;
		static constexpr size_t ArgCount = std::tuple_size_v<Tuple>;

		static napi_value invoke(napi_env env, napi_callback_info info) {
			// Get callback information.
			//
			engine	  ctx						= {env};
			void*		  data					= nullptr;
			napi_value args[ArgCount + 1] = {nullptr};
			size_t	  argc					= ArgCount;
			if (auto s = napi_get_cb_info(env, info, &argc, &args[1], &args[0], &data); s != napi_ok) {
				ctx.throw_error(fmt::concat("Invalid invocation:", s));
				return nullptr;
			}

			// Convert all arguments and invoke the function, propagate any exception.
			//
			try {
				auto get_functor = [&]() -> decltype(auto) {
					if constexpr (StatelessCallable<F>) {
						return F{};
					} else if constexpr (std::is_trivially_destructible_v<F> && sizeof(F) == sizeof(void*)) {
						return bitcast<F>(data);
					} else {
						return (*(F*) data);
					}
				};

				Tuple				tuple	  = {};
				decltype(auto) functor = get_functor();
				detail::pack_tuple<0>(tuple, env, &args[HasThis ? 0 : 1]);

				if constexpr (Async) {
					return detail::apply_async<R, Tuple>(env, functor, std::move(tuple));
				} else {
					if constexpr (!std::is_same_v<R, void>) {
						return value::make(env, std::apply(functor, std::move(tuple)));
					} else {
						std::apply(functor, std::move(tuple));
						return value::make(env, std::nullopt);
					}
				}
			} catch (std::exception ex) {
				ctx.throw_error(ex.what());
			}
			return nullptr;
		}

		template<typename T>
		static callback_details create(const engine& env, std::string_view name, T&& fn) {
			// Create the function and return.
			//
			void*			  data;
			napi_finalize finalizer;
			if constexpr (StatelessCallable<F>) {
				data		 = nullptr;
				finalizer = nullptr;
			} else if constexpr (std::is_trivially_destructible_v<F> && sizeof(F) == sizeof(void*)) {
				data		 = bitcast<void*>(fn);
				finalizer = nullptr;
			} else {
				data		 = new F(std::forward<T>(fn));
				finalizer = [](napi_env e, void* data, void*) { delete (F*) data; };
			}
			return {&invoke, finalizer, data};
		}
	};
	template<bool HasThis, bool Async, typename F>
	static callback_details create_callback(const engine& env, std::string_view name, F&& fn) {
		return callback_builder<std::decay_t<F>, HasThis, Async>::create<F>(env, name, std::forward<F>(fn));
	}
	template<bool HasThis, bool Async, typename F>
	static value create_function(const engine& env, std::string_view name, F&& fn) {
		auto [callback, finalizer, data] = create_callback<HasThis, Async, F>(env, name, std::forward<F>(fn));
		napi_value result = nullptr;
		env.assert(napi_create_function(env, name.data(), name.size(), callback, data, &result));
		if (finalizer) {
			env.assert(napi_add_finalizer(env, result, data, finalizer, nullptr, nullptr));
		}
		return {env, result};
	}

	// Define the function type.
	//
	struct function : value {
		function()										  = default;
		function(function&&) noexcept				  = default;
		function(const function&)					  = default;
		function& operator=(function&&) noexcept = default;
		function& operator=(const function&)	  = default;
		function(value v) : value(v) {}
		function(napi_env env, napi_value val) : value(env, val) {}
		function& operator=(value v) {
			value::val = v.val;
			value::env = v.env;
			return *this;
		};

		// Creates a function.
		//
		template<bool HasThis = false, typename F>
		static function make(const engine& env, std::string_view name, F&& fn) {
			auto val = create_function<HasThis, false, F>(env, name, std::forward<F>(fn));
			return function{val, val};
		}
		template<bool HasThis = false, typename F>
		static function make_async(const engine& env, std::string_view name, F&& fn) {
			auto val = create_function<HasThis, true, F>(env, name, std::forward<F>(fn));
			return function{val, val};
		}

		// Invocation.
		//
		template<typename... Tx>
		value invoke(const Tx&... a) {
			napi_value args[sizeof...(Tx)];
			detail::unpack_tuple<0>(std::tie(a...), value::env, args);

			
			napi_value	global = nullptr;
			napi_get_global(value::env, &global);

			napi_value result = nullptr;
			value::env.assert(napi_call_function(value::env, global, value::val, std::size(args), &args[0], &result));
			return {value::env, result};
		}
		template<typename... Tx>
		value mem_invoke(const Tx&... a) {
			napi_value args[sizeof...(Tx)];
			detail::unpack_tuple<0>(std::tie(a...), value::env, args);

			napi_value result = nullptr;
			value::env.assert(napi_call_function(value::env, args[0], value::val, std::size(args) - 1, &args[1], &result));
			return {value::env, result};
		}
	};

	// Define the typedecl type.
	//
	struct typedecl {
		using engine_type = engine;

		reference									  ctor = {};
		std::vector<napi_property_descriptor> properties;

		static typedecl* fetch(const engine& e, u32 id);
		template<typename T>
		static typedecl* fetch(const engine& e) {
			return fetch(e, bind::user_class<T>::api_id);
		}
	};

	// Define the local context type.
	//
	struct local_context {
		napi_env __parent;

		// Cached values.
		//
		reference symbol_iterator;
		reference return_self;

		// Threadsafe function details.
		//
		napi_threadsafe_function sync_fn;
		size_t						 sync_thread = 0;

		// Type decls.
		//
		typedecl types[1];

		// Ctor/Dtor for handling variable size data.
		//
		inline static thread_local local_context* cache = nullptr;
		local_context(napi_env p) : __parent(p) {
			RC_ASSERT(!cache);
			cache = this;
			std::uninitialized_default_construct_n(&types[1], next_api_type_id - 1);
		}
		~local_context() {
			RC_ASSERT(cache == this);
			cache = nullptr;
			std::destroy_n(&types[1], next_api_type_id - 1);
		}

		// Initializer and getter.
		//
		static local_context* get_ext(const engine& e) {
			local_context* ctx = nullptr;
			napi_get_instance_data(e, (void**) &ctx);
			RC_ASSERT(ctx);
			return ctx;
		}
		static local_context* get(const engine& e) {
			local_context* ctx = cache;
			RC_ASSERT(ctx && ctx->__parent == e.env);
			return ctx;
		}
		static void init(const engine& e) {
			// Allocate and set the context.
			//
			size_t			len = sizeof(local_context) + sizeof(typedecl) * (next_api_type_id - 1);
			local_context* ctx = new (operator new(len)) local_context(e);
			napi_set_instance_data(
				 e, ctx, +[](napi_env, void* data, void*) { delete (local_context*) data; }, nullptr);

			// Fetch the cached variables.
			//
			ctx->symbol_iterator = e.globals().get("Symbol").template as<js::object>().get("iterator");
			ctx->return_self		= js::function::make<true>(e, "", [](js::value self) { return self; });

			// Create the threadsafe function for sync.
			//
			auto sync_worker = [](napi_env env, napi_value cb, void* ctx, void* data) {
				RC_ASSERT(env == ctx);

				auto p = uptr(data);
				if (p & 1) {
					auto* f = (std::function<void()>*) (p & ~1ull);
					(*f)();
					delete f;
				} else {
					auto* f = (function_view<void()>*) (p & ~1ull);
					(*f)();
				}
			};
			ctx->sync_thread = platform::thread_id();
			e.assert(napi_create_threadsafe_function(e, nullptr, nullptr, value::make(e, "retro-ipi"), 0, 1, nullptr, nullptr, e.env, +sync_worker, &ctx->sync_fn));
			//e.assert(napi_unref_threadsafe_function(e, ctx->sync_fn));
		}
	};
	inline typedecl* typedecl::fetch(const engine& e, u32 id) { return &local_context::get(e)->types[id]; }
	template<typename T>
	inline bool value::isinstance() const {
		bool res = false;
		napi_instanceof(env, val, typedecl::fetch<T>(env)->ctor.lock(), &res);
		return res;
	}

	// Sync token.
	//
	struct sync_token {
		using engine_type = engine;

		napi_env						 env = nullptr;
		local_context*				 lctx = nullptr;

		// Null and value constructors.
		//
		constexpr sync_token() = default;
		sync_token(napi_env e) : env(e), lctx(local_context::get(e)) {
			// napi_acquire_threadsafe_function(lctx->sync_fn);
		}

		// Move construction and assignment.
		//
		constexpr sync_token(sync_token&& o) noexcept { swap(o); }
		constexpr sync_token& operator=(sync_token&& o) noexcept {
			swap(o);
			return *this;
		}
		constexpr void swap(sync_token& o) {
			std::swap(o.env, env);
			std::swap(o.lctx, lctx);
		}

		// Implement the interface.
		//
		engine context() const { return env; }
		template<typename F>
		decltype(auto) call(F&& fn) const {
			using R = decltype(fn());

			if constexpr (std::is_void_v<R>) {
				RC_ASSERT(lctx);
				if (lctx->sync_thread == platform::thread_id()) [[unlikely]] {
					fn();
					return;
				}

				std::atomic<bool> done = false;

				function_view<void()> fv = [&]() {
					fn();
					done.store(1);
					done.notify_one();
				};
				RC_ASSERT(napi_call_threadsafe_function(lctx->sync_fn, &fv, napi_tsfn_blocking) == napi_ok);
				while (!done.load(std::memory_order::relaxed)) {
					done.wait(false);
				}
			}
			else {
				std::optional<R> result;
				call([&]() { result.emplace(fn()); });
				return std::move(result).value();
			}
		}
		template<typename F>
		void queue(F&& fn) const {
			RC_ASSERT(lctx);
			auto fp = new std::function<void()>(std::forward<F>(fn));
			RC_ASSERT(napi_call_threadsafe_function(lctx->sync_fn, (void*) (uptr(fp) | 1), napi_tsfn_blocking) == napi_ok);
		}
		template<typename F>
		decltype(auto) operator()(F&& fn) const {
			return call(std::forward<F>(fn));
		}

		// Deref on destruction.
		//
		void reset() {
			if (lctx) {
				//napi_release_threadsafe_function(lctx->sync_fn, napi_tsfn_release);
			}
			lctx = nullptr;
			env  = nullptr;
		}
		~sync_token() { reset(); }
	};

	// Class utilities.
	//
	inline napi_value default_ctor(napi_env env, napi_callback_info ci) {
		napi_value self;
		engine	  ctx{env};
		ctx.assert(napi_get_cb_info(env, ci, nullptr, nullptr, &self, nullptr));
		return self;
	}
	inline void set_parent_class(napi_env env, napi_value ctor, napi_value super_ctor) {
		napi_value global, global_object, set_proto, ctor_proto_prop, super_ctor_proto_prop;
		napi_value args[2];

		napi_get_global(env, &global);
		napi_get_named_property(env, global, "Object", &global_object);
		napi_get_named_property(env, global_object, "setPrototypeOf", &set_proto);
		napi_get_named_property(env, ctor, "prototype", &ctor_proto_prop);
		napi_get_named_property(env, super_ctor, "prototype", &super_ctor_proto_prop);

		args[0] = ctor_proto_prop;
		args[1] = super_ctor_proto_prop;
		napi_call_function(env, global, set_proto, 2, args, nullptr);

		args[0] = ctor;
		args[1] = super_ctor;
		napi_call_function(env, global, set_proto, 2, args, nullptr);
	}

	// Pointer utilities.
	//
	inline static napi_type_tag weakptr_tag	= {1, 1};
	inline static napi_type_tag strongptr_tag = {2, 1};
	inline static napi_type_tag iface_tag		= {3, 1};

	static bool is_smartptr(const value& val) {
		bool res = false;
		napi_check_object_type_tag(val, val, &strongptr_tag, &res);
		if (!res)
			napi_check_object_type_tag(val, val, &weakptr_tag, &res);
		return res;
	}
	static bool is_iface(const value& val) {
		bool res = false;
		napi_check_object_type_tag(val, val, &iface_tag, &res);
		return res;
	}

	inline void ref_finalizer(napi_env, void* data, void*) { rc_header::from(data)->dec_ref(); }
	inline void weak_finalizer(napi_env, void* data, void*) { rc_header::from(data)->dec_ref_weak(); }

	template<typename T>
	inline void unique_finalizer(napi_env, void* data, void*) { delete ((T*)data); }


	template<typename T>
	static value make_ptr(const engine& context, T* ptr, napi_finalize finalizer = nullptr) {
		if (!ptr)
			return value::make(context, std::nullopt);
		napi_value result = nullptr;
		if constexpr (std::is_final_v<T> || !DynUserClass<T>)
			context.assert(napi_new_instance(context, typedecl::fetch<T>(context)->ctor.lock(), 0, nullptr, &result));
		else
			context.assert(napi_new_instance(context, typedecl::fetch(context, ptr->get_api_id())->ctor.lock(), 0, nullptr, &result));
		context.assert(napi_wrap(context, result, ptr, finalizer, nullptr, nullptr));
		return {context, result};
	}
	template<typename T>
	static value make_uptr(const engine& context, std::unique_ptr<T> ptr) {
		if (!ptr)
			return value::make(context, std::nullopt);
		value result = make_ptr(context, ptr.release(), unique_finalizer<T>);
		return result;
	}
	template<typename T>
	static value make_ref(const engine& context, ref<T> ptr) {
		if (!ptr)
			return value::make(context, std::nullopt);
		value result = make_ptr(context, ptr.release(), ref_finalizer);
		RC_ASSERT(napi_type_tag_object(context, result, &strongptr_tag) == napi_ok);
		return result;
	}
	template<typename T>
	static value make_weak(const engine& context, weak<T> ptr) {
		if (!ptr)
			return value::make(context, std::nullopt);
		value result = make_ptr(context, ptr.release(), weak_finalizer);
		RC_ASSERT(napi_type_tag_object(context, result, &weakptr_tag) == napi_ok);
		return result;
	}
	template<typename T>
	static T* get_ptr(const value& val, bool expired_ok = false) {
		if (val.type() != napi_object) {
			throw std::runtime_error(fmt::concat("Expected ", get_type_name<T>()));
		}
		T* r = nullptr;
		napi_unwrap(val, val, (void**) &r);
		if (!r) {
			throw std::runtime_error(fmt::concat("Invalid instantiation of ", get_type_name<T>()));
		}

		if (!expired_ok) {
			bool is_weak = false;
			napi_check_object_type_tag(val, val, &weakptr_tag, &is_weak);
			if (is_weak) {
				if ((rc_header::from(r)->ref_counter & bit_mask(32)) == 0) {
					throw std::runtime_error(fmt::concat("Expired reference to ", get_type_name<T>()));
				}
			}
		}
		return r;
	}

	
	template<typename T>
	static value make_iface(const engine& context, T* ptr) {
		if (!ptr)
			return value::make(context, std::nullopt);

		
		napi_value result = nullptr;
		context.assert(napi_new_instance(context, typedecl::fetch<T>(context)->ctor.lock(), 0, nullptr, &result));
		context.assert(napi_wrap(context, result, (void*)(uptr)(u32) ptr->get_handle(), nullptr, nullptr, nullptr));
		RC_ASSERT(napi_type_tag_object(context, result, &iface_tag) == napi_ok);
		return {context, result};
	}
	template<typename T>
	static interface::handle_type<T> get_iface(const value& val) {
		if (val.type() != napi_object || !is_iface(val)) {
			return {};
		}

		uptr r = 0;
		napi_unwrap(val, val, (void**) &r);
		if (!r) {
			throw std::runtime_error(fmt::concat("Invalid instantiation of ", get_type_name<T>()));
		}
		return interface::handle_type<T>(u32(r));
	}

	// Define the prototype type.
	//
	struct prototype {
		using engine_type = engine;

		engine										  env;
		std::string									  class_name;
		std::vector<napi_property_descriptor> properties;
		napi_callback								  constructor = &default_ctor;
		typedecl*									  super		  = nullptr;

		napi_property_descriptor* add_descriptor() {
			auto r = &properties.emplace_back();
			memset(r, 0, sizeof(napi_property_descriptor));
			return r;
		}

		template<typename T>
		void set_super() {
			RC_ASSERT(!super);
			super = typedecl::fetch<T>(env);
			RC_ASSERT(super);
		}

		template<typename T>
		void make_iterable(T&&) {
			static_assert(StatelessCallable<std::decay_t<T>>);

			constexpr auto fn = [](js::value b) {
				// Ask the given method for the range.
				//
				using Self = std::tuple_element_t<0, typename function_traits<std::decay_t<T>>::arguments>;
				auto range = T{}(b.template as<Self>());
				using Iterator = decltype(range.begin());

				// Make a temporary table to re-use, create the closure with a reference to to self.
				auto tmp = js::object::make(b.env, 3);

				auto fn = function::make(b.env, "iterator.next", [_ = js::reference{b}, tmp = js::reference{tmp}, rng = std::move(range), it = std::optional<Iterator>{}]() mutable {
					if (!it) {
						it = rng.begin();
					}

					js::object result = tmp.lock();
					result.set("done", it.value() == rng.end());
					if (it.value() != rng.end()) {
						result.set("value", *it.value()++);
					} else {
						result.set("value", std::nullopt);
					}
					return result;
				});
				tmp.set("next", fn);
				return tmp;
			};

			auto [cb, _, __] = js::create_callback<true, false>(env, "iterator", fn);
			auto desc		  = add_descriptor();
			desc->name		  = local_context::get(env)->symbol_iterator.lock();
			desc->method	  = cb;
			desc->attributes = napi_property_attributes(napi_default_method);
		}

		template<typename T>
		void add_method(const char* name, T&& fn) {
			auto [cb, _, __] = create_callback<true, false, T>(env, name, std::forward<T>(fn));
			static_assert(StatelessCallable<std::decay_t<T>>);

			auto desc		  = add_descriptor();
			desc->utf8name	  = name;
			desc->method	  = cb;
			desc->attributes = napi_property_attributes(napi_default_method);
		}
		template<typename T>
		void add_static_method(const char* name, T&& fn) {
			auto [cb, _, __] = create_callback<false, false, T>(env, name, std::forward<T>(fn));
			static_assert(StatelessCallable<std::decay_t<T>>);

			auto desc		  = add_descriptor();
			desc->utf8name	  = name;
			desc->method	  = cb;
			desc->attributes = napi_property_attributes(napi_default_method | napi_static);
		}
		template<typename T>
		void add_async_method(const char* name, T&& fn) {
			auto [cb, _, __] = create_callback<true, true, T>(env, name, std::forward<T>(fn));
			static_assert(StatelessCallable<std::decay_t<T>>);

			auto desc		  = add_descriptor();
			desc->utf8name	  = name;
			desc->method	  = cb;
			desc->attributes = napi_property_attributes(napi_default_method);
		}
		template<typename T>
		void add_async_static_method(const char* name, T&& fn) {
			auto [cb, _, __] = create_callback<false, true, T>(env, name, std::forward<T>(fn));
			static_assert(StatelessCallable<std::decay_t<T>>);

			auto desc		  = add_descriptor();
			desc->utf8name	  = name;
			desc->method	  = cb;
			desc->attributes = napi_property_attributes(napi_default_method | napi_static);
		}
		template<typename T>
		void add_static(const char* name, const T& value) {
			auto val = value::make(env, value);

			auto desc		  = add_descriptor();
			desc->utf8name	  = name;
			desc->value		  = val;
			desc->attributes = napi_property_attributes(napi_default_jsproperty | napi_static);
		}
		template<typename T>
		void add_property(const char* name, T&& getter) {
			auto [cbg, _, __] = create_callback<true, false, T>(env, "get_" + std::string(name), std::forward<T>(getter));
			static_assert(StatelessCallable<std::decay_t<T>>);

			auto desc		  = add_descriptor();
			desc->utf8name	  = name;
			desc->getter	  = cbg;
			desc->attributes = napi_property_attributes(napi_enumerable | napi_configurable);
		}
		template<typename Tg, typename Ts>
		void add_property(const char* name, Tg&& getter, Ts&& setter) {
			auto [cbg, _, __]		 = create_callback<true, false, Tg>(env, "get_" + std::string(name), std::forward<Tg>(getter));
			auto [cbs, ___, ____] = create_callback<true, false, Ts>(env, "set_" + std::string(name), std::forward<Ts>(setter));
			static_assert(StatelessCallable<std::decay_t<Tg>>);
			static_assert(StatelessCallable<std::decay_t<Ts>>);

			auto desc		  = add_descriptor();
			desc->utf8name	  = name;
			desc->getter	  = cbg;
			desc->setter	  = cbs;
			desc->attributes = napi_property_attributes(napi_enumerable | napi_configurable | napi_writable);
		}
		template<auto V>
		void add_field_ro(const char* name) {
			return add_property(name, getter<V>);
		}
		template<auto V>
		void add_field_rw(const char* name) {
			return add_property(name, getter<V>, setter<V>);
		}
		bool has_member(const char* name) {
			return range::contains_if(properties, [&](napi_property_descriptor& d) {
				return d.utf8name && !strcmp(d.utf8name, name);
			});
		}

		template<typename T>
		typedecl* write() {
			// Add ref-counter details.
			//
			add_property("refcount", [](bind::js::value p) -> u32 {
				if (is_smartptr(p))
					return u32(rc_header::from(bind::js::get_ptr<T>(p, true))->ref_counter & bit_mask(32));
				else
					return 1;
			});
			add_property("unique", [](bind::js::value p) {
				if (is_smartptr(p))
					return (rc_header::from(bind::js::get_ptr<T>(p, true))->ref_counter & bit_mask(32)) == 1;
				else
					return true;
			});
			add_property("expired", [](bind::js::value p) {
				if (is_smartptr(p))
					return (rc_header::from(bind::js::get_ptr<T>(p, true))->ref_counter & bit_mask(32)) == 0;
				else
					return false;
			});

			// Add interface details.
			//
			if constexpr (interface::Instance<T>) {
				add_property("name", [](T::handle_t h) {
					return h.get_name();
				});
				add_static_method("lookup", [](std::string name) {
					return T::lookup(name);
				});
				add_method("equals", [](T::handle_t h, const js::value& other) {
					if (other.template is<typename T::handle_t>()) {
						return other.template as<typename T::handle_t>() == h;
					}
					return false;
				});
				add_property("comperator", [](T::handle_t h) {
					return h.value;
				});
			} else {
				if (!has_member("equals")) {
					add_method("equals", [](T* p, const js::value& other) {
						if (other.template is<T*>()) {
							return other.template as<T*>() == p;
						}
						return false;
					});
				}
				if (!has_member("comperator")) {
					add_property("comperator", [](T* p) { return make_comperator(p); });
				}
			}

			// Inherit properties of parents if relevant.
			//
			if (super) {
				for (auto& prop : super->properties) {
					auto match = range::find_if(properties, [&](napi_property_descriptor& d) {
						if (d.utf8name) {
							return prop.utf8name && !strcmp(d.utf8name, prop.utf8name);
						} else if (d.name && prop.name) {
							bool eq = false;
							napi_strict_equals(env, prop.name, d.name, &eq);
							return eq;
						}
						return false;
					});
					if (match == properties.end()) {
						properties.emplace_back(prop);
					}
				}
			}

			// Define the class and write to the slot.
			//
			napi_value result;
			env.assert(napi_define_class(env, class_name.c_str(), NAPI_AUTO_LENGTH, constructor, nullptr, properties.size(), properties.data(), &result));
			auto* decl = typedecl::fetch<T>(env);
			decl->ctor = value{env, result};
			decl->properties = std::move(properties);

			// Inherit from parent if there is one.
			//
			if (super) {
				RC_ASSERT(super->ctor);
				set_parent_class(env, result, super->ctor.lock());
			}
			return decl;
		}
	};
};

namespace retro::bind {
	// Define primitive converters.
	//
	// std::nullopt_t
	template<>
	struct converter<js::engine, std::nullopt_t> {
		bool is(const js::value& val) const {
			auto t = val.type();
			return t == napi_undefined || t == napi_null;
		}
		js::value	from(const js::engine& context, std::nullopt_t) const {
			napi_value result;
			context.assert(napi_get_null(context, &result));
			return js::value(context, result);
		}
		std::nullopt_t as(const js::value& val, bool coerce) const {
			if (!coerce && !is(val)) {
				throw std::runtime_error("Expected null");
			}
			return std::nullopt;
		}
	};
	//  bool
	//  i8,i16,i32,i64,i52 TODO: i128
	//  u8,u16,u32,u64     TODO: u128
	//  f32,f64,f80
	template<typename T>
		requires(std::is_integral_v<T> || std::is_floating_point_v<T>)
	struct converter<js::engine, T> {
		bool is(const js::value& val) const {
			auto t = val.type();
			if constexpr (std::is_floating_point_v<T>)
				return t == napi_number;
			else
				return t == napi_boolean || t == napi_number || t == napi_bigint;
		}

		js::value from(const js::engine& env, T v) const {
			napi_status status;
			js::value	res{env, nullptr};

			if constexpr (std::is_floating_point_v<T>)
				status = napi_create_double(env, f64(v), &res.val);
			else if constexpr (std::is_same_v<T, i64>)
				status = napi_create_bigint_int64(env, v, &res.val);
			else if constexpr (std::is_same_v<T, u64>)
				status = napi_create_bigint_uint64(env, v, &res.val);
			else if constexpr (std::is_same_v<T, bool>)
				status = napi_get_boolean(env, v, &res.val);
			else if constexpr (std::is_signed_v<T>)
				status = napi_create_int32(env, v, &res.val);
			else
				status = napi_create_uint32(env, v, &res.val);
			env.assert(status);
			return res;
		}

		auto try_cvt(const js::value& val) const {
			if constexpr (std::is_floating_point_v<T>) {
				double result;
				return std::pair{napi_get_value_double(val, val, &result), (T) result};
			} else if constexpr (sizeof(T) == 8 && std::is_signed_v<T>) {
				if (val.type() == napi_number) {
					i64 result = 0;
					napi_status status = napi_get_value_int64(val, val, &result);
					return std::pair{status, (T) result};
				} else {
					i64			result	= 0;
					bool			lossless = true;
					napi_status status	= napi_get_value_bigint_int64(val, val, &result, &lossless);
					if (!lossless)
						status = napi_generic_failure;
					return std::pair{status, (T) result};
				}
			} else if constexpr (sizeof(T) == 8 && std::is_unsigned_v<T>) {
				if (val.type() == napi_number) {
					i64			result = 0;
					napi_status status = napi_get_value_int64(val, val, &result);
					if (result >= 0)
						return std::pair{status, (T) result};
				} 
				u64			result	= 0;
				bool			lossless = true;
				napi_status status	= napi_get_value_bigint_uint64(val, val, &result, &lossless);
				if (!lossless)
					status = napi_generic_failure;
				return std::pair{status, (T) result};
			} else if constexpr (std::is_same_v<T, bool>) {
				bool result;
				return std::pair{napi_get_value_bool(val, val, &result), (T) result};
			} else if constexpr (std::is_signed_v<T>) {
				i32 result;
				return std::pair{napi_get_value_int32(val, val, &result), (T) result};
			} else {
				u32 result;
				return std::pair{napi_get_value_uint32(val, val, &result), (T) result};
			}
		}
		T as(js::value val, bool coerce = false) const {
			auto [status, result] = try_cvt(val);
			if (status != napi_ok && coerce) {
				if constexpr (std::is_same_v<T, bool>)
					val.env.assert(napi_coerce_to_bool(val, val, &val.val));
				else
					val.env.assert(napi_coerce_to_number(val, val, &val.val));
				std::tie(status, result) = try_cvt(val);
			}
			if (status != napi_ok) {
				throw std::runtime_error(fmt::concat("Expected ", get_type_name<T>()));
			}
			return result;
		}
	};
	template<>
	struct converter<js::engine, i52> {
		bool is(const js::value& val) const {
			auto t = val.type();
			return t == napi_boolean || t == napi_number;
		}
		js::value from(const js::engine& env, i52 v) const {
			js::value	res{env, nullptr};
			env.assert(napi_create_int64(env, v, &res.val));
			return res;
		}
		i52 as(js::value val, bool coerce = false) const { return (i52) val.template as<i64>(coerce); }
	};
	// std::string_view
	template<>
	struct converter<js::engine, std::string_view> {
		bool is(const js::value& val) const {
			return val.type() == napi_string;
		}
		js::value from(const js::engine& context, std::string_view val) const {
			napi_value result;
			context.assert(napi_create_string_utf8(context, val.data(), val.size(), &result));
			return js::value(context, result);
		}
		std::string as(js::value val, bool coerce) const {
			if (!is(val)) {
				if (!coerce || napi_coerce_to_string(val, val, &val.val) != napi_ok) {
					throw std::runtime_error("Expected string");
				}
			}

			size_t length = 0;
			napi_get_value_string_utf8(val, val, nullptr, 0, &length);

			std::string out(length, '\x0');
			val.env.assert(napi_get_value_string_utf8(val, val, (char*) out.data(), length + 1, &length));
			return out;
		}
	};
	// std::span<const u8>
	template<>
	struct converter<js::engine, std::span<const u8>> {
		bool is(const js::value& val) const {
			bool res = false;
			if (val.val)
				napi_is_buffer(val.env, val.val, &res);
			return res;
		}

		js::value from(const js::engine& context, std::span<const u8> val) const {
			void*		  cpy;
			napi_value result = nullptr;
			context.assert(napi_create_buffer_copy(context, val.size(), val.data(), &cpy, &result));
			return {context, result};
		}

		std::span<const u8> as(const js::value& val, bool) const {
			if (!is(val)) {
				throw std::runtime_error("Expected Buffer");
			}

			void*	 data	  = nullptr;
			size_t length = 0;
			val.env.assert(napi_get_buffer_info(val, val, &data, &length));
			return {(const u8*) data, (u8*) data + length};
		}
	};
	template<>
	struct converter<js::engine, std::vector<u8>> {
		bool is(const js::value& val) const {
			bool res = false;
			if (val.val)
				napi_is_buffer(val.env, val.val, &res);
			return res;
		}

		js::value from(const js::engine& context, const std::vector<u8>& val) const {
			void*		  cpy;
			napi_value result = nullptr;
			context.assert(napi_create_buffer_copy(context, val.size(), val.data(), &cpy, &result));
			return {context, result};
		}

		std::vector<u8> as(const js::value& val, bool) const {
			if (!is(val)) {
				throw std::runtime_error("Expected Buffer");
			}
			void*	 data	  = nullptr;
			size_t length = 0;
			val.env.assert(napi_get_buffer_info(val, val, &data, &length));
			return std::vector<u8>{(u8*) data, (u8*) data + length};
		}
	};
	// range::subrange<T>
	template<typename Iterator>
	struct converter<js::engine, range::subrange<Iterator>> {
		js::value from(const js::engine& context, const range::subrange<Iterator>& rng) const {
			// Make a temporary table to re-use, create the closure with a reference to to self.
			auto tmp = js::object::make(context, 3);

			auto fn = js::function::make(context.env, "iterator.next", [tmp = js::reference{tmp}, rng = std::move(rng), it = std::optional<Iterator>{}]() mutable {
				if (!it) {
					it = rng.begin();
				}
				js::object result = tmp.lock();
				result.set("done", it.value() == rng.end());
				if (it.value() != rng.end()) {
					result.set("value", *it.value()++);
				} else {
					result.set("value", std::nullopt);
				}
				return result;
			});

			tmp.set("next", fn);
			auto* ctx = js::local_context::get(context);
			tmp.set(ctx->symbol_iterator.lock(), ctx->return_self.lock());
			return tmp;
		}
	};
	// neo::promise<T>
	template<typename T>
	struct converter<js::engine, neo::promise<T>> {
		js::value from(const js::engine& context, const neo::promise<T>& pr) const {
			napi_deferred def;
			napi_value	  result;
			context.assert(napi_create_promise(context, &def, &result));

			pr.and_finally([=, tk = js::sync_token(context)](const neo::promise<T>& pr) {
				try {

					if constexpr (std::is_void_v<T>) {
						pr.get();
						tk.queue([=, env = tk.context()] { env.assert(napi_resolve_deferred(env, def, js::value::make(env, std::nullopt))); });
					} else {
						auto* resptr = &pr.get();
						tk.queue([=, _=pr, env = tk.context()] { env.assert(napi_resolve_deferred(env, def, js::value::make(env, *resptr))); });
					}
				} catch (const std::exception& ex) {
					tk.queue([env = tk.context(), def, result = std::string(ex.what())] { env.assert(napi_reject_deferred(env, def, js::value::make(env, result))); });
				}
			});
			return js::value(context, result);
		}
	};

	// User classes.
	//
	template<UserClass T>
	struct converter<js::engine, T*> {
		bool		 is(const js::value& val) const { return val.type() == napi_object; }
		js::value from(const js::engine& context, T* val) const {
			if constexpr (std::is_base_of_v<force_rc_t, type_descriptor<T>>) {
				return js::make_ref(context, ref<T>(val));
			} else {
				return js::make_ptr(context, val);
			}
		}
		T*			 as(const js::value& val, bool) const { return js::get_ptr<T>(val); }
	};
	template<UserClass T>
	struct converter<js::engine, std::unique_ptr<T>> {
		js::value from(const js::engine& context, std::unique_ptr<T> val) const { return js::make_uptr(context, std::move(val)); }
	};
	template<UserClass T>
	struct converter<js::engine, T> {
		bool is(const js::value& val) const { return val.type() == napi_object; }
		template<typename Ty>
		js::value from(const js::engine& context, Ty&& val) const {
			return js::make_uptr<T>(context, std::make_unique<T>(std::forward<Ty>(val)));
		}
		T& as(const js::value& val, bool) const { return *js::get_ptr<T>(val); }
	};
	template<UserClass T>
	struct converter<js::engine, ref<T>> {
		bool		 is(const js::value& val) const { return val.type() == napi_object && js::is_smartptr(val); }
		js::value from(const js::engine& context, const ref<T>& val) const { return js::make_ref(context, val); }
		ref<T>	 as(const js::value& val, bool) const { return js::get_ptr<T>(val); }
	};
	template<UserClass T>
	struct converter<js::engine, weak<T>> {
		bool		 is(const js::value& val) const { return val.type() == napi_object && js::is_smartptr(val); }
		js::value from(const js::engine& context, const weak<T>& val) const { return js::make_weak(context, val); }
		weak<T>	 as(const js::value& val, bool) const { return js::get_ptr<T>(val, true); }
	};

	// Interfaces.
	//
	template<typename T>
	struct converter<js::engine, interface::handle_type<T>> {
		using handle_t = interface::handle_type<T>;

		bool is(const js::value& val) const {
			auto t = val.type();
			return t == napi_null || (t == napi_object && js::is_iface(val));
		}
		js::value from(const js::engine& context, handle_t val) const { return js::make_iface(context, val.get()); }
		handle_t	 as(const js::value& val, bool) const { return js::get_iface<T>(val); }
	};

	// Define value-type converters.
	//
	template<>
	struct converter<js::engine, napi_value> {
		bool		  is(const js::value& val) const { return true; }
		js::value  from(const js::engine& context, const napi_value& val) const { return {context, val}; }
		napi_value as(const js::value& val, bool) const { return val; }
	};
	template<>
	struct converter<js::engine, js::reference> {
		bool			  is(const js::value& val) const { return true; }
		js::value	  from(const js::engine& context, const js::reference& val) const { return val.lock(); }
		js::reference as(const js::value& val, bool) const { return val; }
	};
	template<>
	struct converter<js::engine, js::value> {
		bool		 is(const js::value& val) const { return true; }
		js::value from(const js::engine& context, const js::value& val) const { return val; }
		js::value as(const js::value& val, bool) const { return val; }
	};
	template<>
	struct converter<js::engine, js::array> {
		using T = js::array;

		bool is(const js::value& val) const {
			bool result = false;
			if (val)
				napi_is_array(val, val, &result);
			return result;
		}
		js::value from(const js::engine& context, const js::array& val) const { return val; }
		js::array as(const js::value& val, bool) const {
			if (!is(val)) {
				throw std::runtime_error("Expected any[]");
			}
			return {val, val};
		}
	};
	template<>
	struct converter<js::engine, js::object> {
		using T = js::array;

		bool is(const js::value& val) const {
			auto t = val.type();
			return t == napi_object || t == napi_function;
		}
		js::value from(const js::engine& context, const js::object& val) const { return val; }

		js::object as(js::value val, bool coerce) const {
			if (!is(val)) {
				if (!coerce || napi_coerce_to_object(val, val, &val.val) != napi_ok)
					throw std::runtime_error("Expected Object");
			}
			return {val, val};
		}
	};
	template<>
	struct converter<js::engine, js::function> {
		using T = js::array;

		bool		 is(const js::value& val) const { return val.type() == napi_function; }
		js::value from(const js::engine& context, const js::function& val) const { return val; }

		js::function as(js::value val, bool coerce) const {
			if (!is(val)) {
				throw std::runtime_error("Expected function");
			}
			return {val, val};
		}
	};
};