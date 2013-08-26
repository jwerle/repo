
#include <v8.h>
#include <node.h>
#include "repo.h"

#define str(s) v8::String::New(s)

class User {
	// repo_user_t *ref_;
	v8::Persistent<v8::Object> obj_;
	public:
		User () {
			// ref_ = repo_user_new();
			obj_->SetPointerInInternalField(0, this);
		}

		v8::Handle<v8::Object>
		ToObject () {
			return obj_;
		}
};

v8::Handle<v8::Value>
NewUser (const v8::Arguments &args) {
	v8::HandleScope scope;
	User *user = new User();

 return scope.Close(user->ToObject());
}

void
Init (v8::Handle<v8::Object> exports) {
	NODE_SET_METHOD(exports, "User", NewUser);
}

NODE_MODULE(repo, Init);