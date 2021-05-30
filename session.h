#ifndef SESSION_H_
#define SESSION_H_

/*

A session is a resource containing somewhat arbitrary state associated
with any active anticent process. It is like the "environment" of a
Windows or Linux process.

The "session identifier" is a token passed as an HTTP cookie which identifies
a normal resource containing data associated with a given session. There is
a one-to-one relationship between session identifiers and session resources,
so the session identifier can just be a unique resource name.

A session is initialized with default information. However, a new session may
be created because the user changed privileges. In that case, the new session
is initialized with (most of) the data from the current session.


A session has an associated cid that sets its default temporal view
of the database. Sessions are of three distinct flavors:

Stateless session: the client is unwilling or unable to support
cookies, so each client request is completely independent of the
others. Creating this type of session requires nothing from the
database except some reasonably recent cid. It has a session ID of 0,
is not stored in the database, and is deleted when its associated
connection is closed.

Transient session: the client supports cookies, so we are able to
store state and offer behavior that is altered by past actions the
client took during that session. The session is stored in the
database, even if there is no associated user login. For example, this
allows a random guest to accumulate items in a shopping cart without
having to create an account, or create and edit a comment.

Sticky session: the client supports cookies and is logged in, which
allows us to tie the session data with more permanent data being
maintained for that user. This allows a system to provide a consistent
view, even across browsers. For example, if the user changes resource
X in browser/machine A and then immediately requests resource X on
browser/machine B, they should see their change correctly
reflected. This also allows building a system that allows multiple
sets of transient data, much like multiple terminal windows on Linux,
where each window can have a distinct: command history, environment
variable set, current directory, etc.

A session must be created or fetched for each HTTP request.



 */

class Session : public Resource
    {
public:
    enum
        {
        STATELESS,
        TRANSIENT,
        PERSISTENT,
        };
    int     type;
    int     uid;
    int     gid;
    cid_t   cid;
    };

#endif /* SESSION_H_ */
