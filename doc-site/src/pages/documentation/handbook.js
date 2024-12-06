// @flow
import React from 'react';
import {Link} from "react-router-dom";

// components
import CodeBlock from "../../components/CodeBlock";
import PageTitle from '../../components/PageTitle';

// diagrams
import HighLevel from '../../assets/images/diagrams/1-high-level.svg'
import WorkerComposition from '../../assets/images/diagrams/2-worker-composition.svg'
import CallSaveData from '../../assets/images/diagrams/3-call-saveData-high-level.svg'
import ServiceMethodCall from '../../assets/images/diagrams/4-service-method-call.svg'
import ClassHeirarchyMessages from '../../assets/images/diagrams/5-class-hierarchy-messages.svg'
import CallSendMessages from '../../assets/images/diagrams/6-call-send-message.svg'

const CoreConceptsPage = (): React$Element<React$FragmentType> => {
    return (
        <><PageTitle
            breadCrumbItems={[
                {label: 'Documentation', path: '/documentation'},
                {label: 'Handbook', path: '/documentation/handbook', active: true}
            ]}
            title={"The Estate Developer's Handbook"}
        />
            <h3 className={'top-h3'}>Introduction</h3>
            This is a guide for software developers to learn the core concepts required to get the most out of the
            Estate development platform.<p/>
            <p/>
            It is assumed that the reader understands the basics of Object-Oriented programming and modern JavaScript/TypeScript (JS/TS)
            concepts like async/await and promises. In addition, it will be helpful to have a basic understanding of
            database terminology to understand what Estate replaces.
            <h3>Estate</h3>
            Estate is a completely re-envisioned, high-level cloud software development platform.
            It’s a very opinionated cloud platform that requires the least amount of
            code to create the most amount of valuable software available to developers today.<p/>
            <p/>
            Estate is not “no-code.” It’s <b>just enough code.</b><p/>
            <em>Estate’s goal is to remove all the boilerplate code and DevOps nonsense back-end devs must write,
                code-generate, and maintain.</em>
            <p/>
            Estate removes the need to:
            <ul>
                <li>Write app-server endpoints and routing logic (HTTP/WebSockets etc.)</li>
                <li>Write complicated and fragile database interaction code (SQL/NoSQL etc.)</li>
                <li>Do anything to make Server-Sent-Events work</li>
                <li>Write the client-side code that talks to backend services (fetch, etc..)</li>
                <li>Duplicate business logic/rules between the backend and frontend codebases (I.e. property rules such
                    as firstName must be a string, not empty, and shorter than 51 characters)
                </li>
                <li>Provision or manage servers/docker containers/Kubernetes stacks</li>
                <li>Deal with stateless services (Estate services are stateful)</li>
            </ul>
            <p/>
            By removing so much unnecessary code, Estate frees up time to add features and write tests which is
            something all businesses, regardless of size, can use more of.
            {/*<h3>But what is it <em>really?</em></h3>*/}
            {/*Estate is all the connected Estate Clusters.*/}
            {/*Each Cluster is a Kubernetes stack.*/}
            {/*<ol>*/}
            {/*    The admin layer:*/}
            {/*    <li>Jayne: account and worker admin, 2nd-step compilation</li>*/}
            {/*    The runtime layer consists of two parts:*/}
            {/*    <li>Serenity: the app/server database, 3rd-step compilation</li>*/}
            {/*    <li>River: the router/validator</li>*/}
            {/*</ol>*/}
            <h3>Workers</h3>
            Estate hosts cloud backends in always-on, <em>internet-accessible machines</em> called <b>Workers</b> that
            your applications can access at runtime using code-generated client classes.
            <div style={{overflowX: 'auto'}}>
                <img src={HighLevel} alt={"Estate High Level Diagram"}
                     style={{marginTop: '20px', width: '800px', height: '400px'}}/>
            </div>
            <p/>Workers:
            <ul>
                <li>Are serverless: No DevOps or management overhead</li>
                <li>Are sandboxed environments with isolated datastores</li>
                <li>Are fully programmable using pure, modern object-oriented TypeScript syntax</li>
                <li>Can handle any number of requests at the same time because unlike Node.js,
                    they’re <b>multi-threaded</b> (without the need to write thread-aware code)
                </li>
                <li>Are very fast: most requests are handled in under 200μs which is 1/5th of 1 millisecond (not
                    including network round-trip)
                </li>
                <li>Are managed using a CLI application so it’s perfect for efficiency-focused software engineers</li>
            </ul>
            <h3>Worker Composition</h3>
            Each worker is composed of two layers: <b>Worker Process</b> and the <b>Worker Kernel.</b><p/>
            <img src={WorkerComposition} alt={"Worker Composition"} className={'diagram-right'}
                 style={{width: '350px', height: '400px'}}/>
            <h4 style={{marginTop: '30px'}}>Worker Process</h4>
            The services and objects you author live in <em>Worker Process</em> at runtime. Worker Process is an <em>Internet-connected
                Inversion of Control container</em> (<b>IIoC</b>). Similar to existing IoC containers like Angular,
            Aurelia, or InversifyJS, except its objects live outside the lifecycle of a given client- they live forever
            (or until you delete them) and, most importantly, can be referenced directly over the Internet.
            <h4>Worker Kernel</h4>
            The Worker Kernel is an object-oriented <em>cloud operating system</em> that manages and provides internet
            access to Worker Process at runtime.
            <p/>
            <p/>
            The Worker Kernel handles the following things for Worker Process:
            <ul>
                <li>Request routing</li>
                <li>Task & thread management</li>
                <li>Real-time networking (WebSockets over HTTPS/TLS)</li>
                <li>Transactional, ACID compliant data storage for services and objects (aka datastores) using the
                    Unit-of-Work methodology
                </li>
                <li>Hardware monitoring and fault mitigation</li>
                <li>Usage-based autoscaling and provisioning</li>
                <li>Datastore synchronization/replication and fault-tolerance (Business-class subscriptions only)</li>
            </ul>
            <p/>
            The Worker Kernel is programmable by Worker developers via the <code>worker-runtime</code> TypeScript library. This
            library is created when the <code>estate worker init</code> command is run.
            <h3>Hosting in Worker Process</h3>
            Hosting a class in Worker Process is easy. Simply extend one of three abstract base
            classes: <b>Service</b>, <b>Data</b>, or <b>Message</b> (not shown, more on this later) found in
            the <code>worker-runtime</code> library. Which abstract class you choose determines how instances of that class
            will behave in Worker Process and how clients use them at runtime.
            <h4>Services - stateful microservices</h4>
            Service instances are global internet-connected singletons. Meaning, any number of client application
            instances can get references to Service shared object instances at the same time.
            <h5>Obtaining service instances</h5>
            A client application uses this TypeScript syntax to get a service instance at runtime:
            <CodeBlock code={`const myService = worker.getService(MyService,"my primary key")`} language={"javascript"}/>
            <p/>The first argument (MyService) is your service class that extends Service. The second argument is
            called the primaryKey. This is an opaque string that when combined with the service class, uniquely
            identifies the service instance living Worker Process.
            <p/>Service instances are created automatically the first time a method is called on them. Additionally,
            they live until you explicitly delete them (see the Worker Runtime API documentation for more details).
            <h5>Executing remote code</h5>
            Once a client has a reference to an instance of a Service in Worker Process, it can make asynchronous calls
            to its methods. Each call routes through Estate and executes on the service object instance in Worker
            Space. At runtime, inside the service method code running in Worker Process, <code>this</code> is the service
            object instance.
            <h5>Units-of-work</h5>
            Each service method call is executed inside a datastore transaction. The service method code that’s executed
            does so as a unit-of-work. As a result, either everything succeeds or no changes are made to any of the
            datastores affected. This allows clients to retry calls without fear of data corruption.
            <h5>Transaction Success or Failure</h5>
            When the service method code is done executing, the following questions are asked to determine whether or
            not the transaction was successful:
            <ol>
                <li>Was there a system-level error?</li>
                <li>Was an unhandled JavaScript exception thrown from user code?</li>
                <li>Were there any unsaved changes to Data instances? (E.g.
                    Changed <code>user._firstName</code> but didn’t call <code>system.saveData(user)</code> )
                </li>
                <li>Were there any concurrent, conflicting modifications to any objects modified by this transaction?
                </li>
            </ol>
            If the answer to any of these questions is yes, the transaction is rolled back causing all object and
            service modifications made as a result of this call to be undone. The client will receive an exception it
            can handle gracefully by retrying the call etc.
            <p/>However, if the answer to all these questions is no, the transaction is committed, the service method
            call is successful, and the results are returned to the client.
            <p/>Services are very useful but their true power is found when using Data to store, retrieve,
            and transfer application data.
            <h4>Data - intelligent data objects</h4>
            <img src={CallSaveData} alt={"Call SaveObject"} className={'diagram-right'}
                 style={{width: '350px', height: '400px'}}/>
            Similar to a database table, you create Data derived classes when you want to persist, use, and share
            structured data.
            <h5>Properties</h5>
            Your Data can contain any number of JavaScript properties with any name, just use normal
            this.propertyName syntax when getting/setting/deleting values. All built-in JavaScript property types are
            supported including Map, Set, and Array. You can create new properties in any Data method, not just
            the constructor.
            <h5>Creating new objects</h5>
            You create new Data object instances the same way you create instances of other TypeScript classes:
            use the new operator. All Data have a primaryKey that, along with the class type, uniquely identifies
            the object instance. The primaryKey is an opaque 1 to 1024 character string and must be supplied to super in
            your class’s constructor.
            <h5>Retrieving existing data objects from the client</h5>
            Data instances can be retrieved from their datastore on the client using this syntax:
            <CodeBlock code={`let myData = await worker.getDataAsync(MyData,"a primary key");`}
                       language={'javascript'}/>
            The first argument is the Data class you’d like to retrieve and the second is the primary key. This
            will load the object into local client memory where it can be displayed in a UI element or modified just
            like any other JavaScript object.
            <h5>Retrieving existing data objects from a service in Worker Process</h5>
            Data instances can also be retrieved from their datastore while inside a Service’s service method
            call with this syntax:
            <CodeBlock code={`let myData = system.getData(MyData,"a primary key");`} language={'javascript'}/>
            The arguments are the same as the clients-side example above. This loads the object into Worker Process memory
            for use during the service method call. You can make changes or use the object like any other JavaScript
            object from inside the service and even return it to the client (using a return statement).
            <h5>Manually Saved</h5>
            Data object property changes must be saved manually from inside a service method (as shown above) or
            directly from the client using the <code>worker.saveDataAsync</code> function. Though the Data
            instances themselves are stored in their datastore, they can be kept track of (or “indexed” in database
            terms) by Services using standard JS/TS collections such as Map, Set, and Array. This allows you to build
            powerful queries by writing plain JS/TS instead of being forced to learn a dedicated query language.
            <h5>In-Proc Method Calls</h5>
            Like Services, Data can contain methods but when a client calls them they’ll execute directly on
            the client. However, if a service calls a Data’s method it will execute in Worker Process. This is an
            essential feature for writing getter/setter style business logic such as enforcing a property’s type or
            having a minimum string length.
            <h5>Remotely observable</h5>
            Data instances are client observable: A client can “watch” a given Data instance for changes made by
            other clients/services and then retrieve those changes in real-time. You accomplish this from a client by
            first subscribing to updates with <code>worker.subscribeUpdatesAsync</code> and then attaching a handler
            function with <code>worker.addUpdateListener</code>. The example application (link below) has a
            demonstration of this functionality.
            <h5>Consistent, transparently versioned</h5>
            Data are transparently versioned and kept consistent using transactions and optimistic concurrency.
            They can be passed to and returned from Service methods and saved as part of other Data instances.
            In fact, there’s a special system method just for saving an entire <a
                href={'https://en.wikipedia.org/wiki/Object_graph'} target={"_blank"}>graph</a> of Data
            instances: <code>system.saveDataGraphs(data...)</code> which takes any number of Data instances
            and saves them along with all the Data instances they refer to (and so on.)
            <h3>Data + Services = combined app & data tier</h3>
            <div style={{overflowX: 'auto'}}>
                <img src={ServiceMethodCall} alt={"Service Method Call"} style={{width: '670px', height: '750px'}}/>
            </div>
            <p/>Putting everything together it would look something like the above diagram. Note that the Data and
            Service base classes require a single argument: the primaryKey value. See the User constructor’s
            super(fullName) call where it runs some business logic on the fullName constructor argument before passing
            it to the Data base class’s constructor (in native code.)
            <p/>As previously noted, Service instances are created on first use. So the first time any service
            method, in this case addUserAsync is called, a new MyService object instance is created with the primary key
            “default” (because that’s what the client specified in the call to <code>worker.getService(MyService,
                "default")</code>).
            <p/>If multiple users access the website they’ll get the very same “default” MyService instance with the
            very same object state (because service instances are singletons.) Clients can share data with each other by
            sharing it with the service and providing a method that returns the property.
            <p/>Allowing clients to interact with each other brings us to the final core concept, Server-Sent-Events.
            <h3>Message - built-in SSE</h3>
            Messages are an implementation of the Server-Sent-Events (SSE) concept used by socket.io and SignalR
            except built into Estate as a first-class concept. Messages are an efficient way to send
            data from the cloud (“Server”) to any number of clients without the clients polling a service.
            <img src={ClassHeirarchyMessages} alt={"Class Heirarchy Messages"} className={'diagram-right'}
                 style={{marginTop: '25px', width: '450px', height: '300px'}}/>
            <h4>Writing & Sending</h4>
            Writing, sending, and consuming messages is easy. To write a Message simply extend the Message abstract
            base class. Unlike the two other base classes, Messages <em>do not</em> have a primaryKey (see below.).
            Then at runtime, from inside a service method call create an instance of your new Message using the new
            operator. Then pass that object to <code>system.sendMessage(...)</code>. Any properties you define/set on the
            Message object will be transported to all clients that have registered to receive messages of that class
            type. Note, presently Messages cannot be fired directly from clients.
            <h4>Short lived, never saved</h4>
            The Message constructor does not take a primaryKey because Messages do not have datastores- their object
            instances are <em>transient</em>. Meaning, they are to be created, fired, and handled by clients but never
            saved or persisted. Think of a message instance as a notification that contains all relevant metadata about
            what happened to cause the message to be sent.
            <h4>Message properties provide context</h4>
            Along with all standard JS/TS types (Map, Set, Array, bool, string, etc..), Data and Service
            instances can be Message properties so you’re free to include all the relevant context when you send message
            instances. For example, a <code>UserAdded</code> message may include a property <code>user</code> that has an
            instance of the <code>User</code> (Data) that was added.
            <h4>Receiving messages on clients</h4>
            <div style={{overflowX: 'auto'}}>
                <img src={CallSendMessages} alt={"Call Send Messages"} style={{width: '900px', height: '450px'}}/>
            </div>
            <p/>This diagram illustrates a message being sent from a service instance and handled by client code.
            <p/>Clients must subscribe to receive messages. To subscribe a client must pass three arguments:
            <ul>
                <li>A source Data or Service (in the above example, <code>this</code> is the current instance
                    of MyService)
                </li>
                <li>The type of the message (<code>MyMessage</code> above)</li>
                <li>A function taking a single argument that will be called with the message instance when a message is
                    recieved
                </li>
            </ul>
            <p/>After a client subscribes and until the client unsubscribes (or shuts down), it will receive messages
            every time <code>system.sendMessage(...)</code> is called with the same source and message type.
            {/* ===== stop ==========   */}
        </>
    );
};

export default CoreConceptsPage;
