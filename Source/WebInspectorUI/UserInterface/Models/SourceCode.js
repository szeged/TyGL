/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

WebInspector.SourceCode = function()
{
    WebInspector.Object.call(this);

    this._originalRevision = new WebInspector.SourceCodeRevision(this, null, false);
    this._currentRevision = this._originalRevision;

    this._sourceMaps = null;
    this._formatterSourceMap = null;
    this._requestContentPromise = null;
};

WebInspector.Object.addConstructorFunctions(WebInspector.SourceCode);

WebInspector.SourceCode.Event = {
    ContentDidChange: "source-code-content-did-change",
    SourceMapAdded: "source-code-source-map-added",
    FormatterDidChange: "source-code-formatter-did-change",
    LoadingDidFinish: "source-code-loading-did-finish",
    LoadingDidFail: "source-code-loading-did-fail"
};

WebInspector.SourceCode.prototype = {
    constructor: WebInspector.SourceCode,
    __proto__: WebInspector.Object.prototype,

    // Public

    get displayName()
    {
        // Implemented by subclasses.
        console.error("Needs to be implemented by a subclass.");
        return "";
    },

    get originalRevision()
    {
        return this._originalRevision;
    },

    get currentRevision()
    {
        return this._currentRevision;
    },

    set currentRevision(revision)
    {
        console.assert(revision instanceof WebInspector.SourceCodeRevision);
        if (!(revision instanceof WebInspector.SourceCodeRevision))
            return;

        console.assert(revision.sourceCode === this);
        if (revision.sourceCode !== this)
            return;

        this._currentRevision = revision;

        this.dispatchEventToListeners(WebInspector.SourceCode.Event.ContentDidChange);
    },

    get content()
    {
        return this._currentRevision.content;
    },

    get contentIsBase64Encoded()
    {
        return this._currentRevision.contentIsBase64Encoded;
    },

    get sourceMaps()
    {
        return this._sourceMaps || [];
    },

    addSourceMap: function(sourceMap)
    {
        console.assert(sourceMap instanceof WebInspector.SourceMap);

        if (!this._sourceMaps)
            this._sourceMaps = [];

        this._sourceMaps.push(sourceMap);

        this.dispatchEventToListeners(WebInspector.SourceCode.Event.SourceMapAdded);
    },

    get formatterSourceMap()
    {
        return this._formatterSourceMap;
    },

    set formatterSourceMap(formatterSourceMap)
    {
        console.assert(this._formatterSourceMap === null || formatterSourceMap === null);
        console.assert(formatterSourceMap === null || formatterSourceMap instanceof WebInspector.FormatterSourceMap);

        this._formatterSourceMap = formatterSourceMap;

        this.dispatchEventToListeners(WebInspector.SourceCode.Event.FormatterDidChange);
    },

    requestContent: function()
    {
        this._requestContentPromise = this._requestContentPromise || this.requestContentFromBackend().then(this._processContent.bind(this));

        return this._requestContentPromise;
    },

    createSourceCodeLocation: function(lineNumber, columnNumber)
    {
        return new WebInspector.SourceCodeLocation(this, lineNumber, columnNumber);
    },

    createLazySourceCodeLocation: function(lineNumber, columnNumber)
    {
        return new WebInspector.LazySourceCodeLocation(this, lineNumber, columnNumber);
    },

    createSourceCodeTextRange: function(textRange)
    {
        return new WebInspector.SourceCodeTextRange(this, textRange);
    },

    // Protected

    revisionContentDidChange: function(revision)
    {
        if (this._ignoreRevisionContentDidChangeEvent)
            return;

        if (revision !== this._currentRevision)
            return;

        this.handleCurrentRevisionContentChange();

        this.dispatchEventToListeners(WebInspector.SourceCode.Event.ContentDidChange);
    },

    handleCurrentRevisionContentChange: function()
    {
        // Implemented by subclasses if needed.
    },

    get revisionForRequestedContent()
    {
        // Implemented by subclasses if needed.
        return this._originalRevision;
    },

    markContentAsStale: function()
    {
        this._contentReceived = false;
    },

    requestContentFromBackend: function()
    {
        // Implemented by subclasses.
        console.error("Needs to be implemented by a subclass.");
        return Promise.reject(new Error("Needs to be implemented by a subclass."));
    },

    // Private

    _processContent: function(parameters)
    {
        // Different backend APIs return one of `content, `body`, or `scriptSource`.
        var content = parameters.content || parameters.body || parameters.scriptSource;
        var error = parameters.error;
        var base64Encoded = parameters.base64Encoded;

        var revision = this.revisionForRequestedContent;

        this._ignoreRevisionContentDidChangeEvent = true;
        revision.content = content || null;
        revision.contentIsBase64Encoded = base64Encoded || false;
        delete this._ignoreRevisionContentDidChangeEvent;

        return Promise.resolve({
            error: error,
            sourceCode: this,
            content: content,
            base64Encoded: base64Encoded
        });
    }
};
