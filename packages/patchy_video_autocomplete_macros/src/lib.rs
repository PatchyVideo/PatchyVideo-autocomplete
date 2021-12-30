use proc_macro::TokenStream;
use quote::quote;
use syn::{parse_macro_input, DeriveInput};

#[proc_macro_derive(FromStrEnum)]
pub fn derive_enum_from_str(input: TokenStream) -> TokenStream {
    let ast = parse_macro_input!(input as DeriveInput);
    let name = &ast.ident;
    let idents = if let syn::Data::Enum(syn::DataEnum { variants, .. }) = &ast.data {
        variants.iter().map(|variant| variant.ident.clone())
    } else {
        panic!("Unsupported indent, only support struct")
    };

    let build_variants = idents.map(|ident| {
        let ident_name = format!("{}", ident);
        quote! {
            #ident_name => Ok(#name::#ident),
        }
    });
    let expanded = quote! {
        impl std::str::FromStr for #name {
            type Err = anyhow::Error;

            fn from_str(s: &str) -> Result<Self, Self::Err> {
                match s {
                    #(#build_variants)*
                    _ => Err(anyhow::anyhow!("Invalid parse error")),
                }
            }
        }
    };

    expanded.into()
}

#[proc_macro_derive(ParseRequest)]
pub fn derive_parse_request(input: TokenStream) -> TokenStream {
    let ast = parse_macro_input!(input as DeriveInput);
    let name = &ast.ident;
    let collection_name = format!("{}Collection", name);
    let collection_ident = syn::Ident::new(&collection_name, name.span());
    let fields = if let syn::Data::Struct(syn::DataStruct {
        fields: syn::Fields::Named(syn::FieldsNamed { ref named, .. }),
        ..
    }) = &ast.data
    {
        named
    } else {
        panic!("Unsupported indent, only support struct")
    };

    let get_next_and_parse = {
        quote! {
            match iter.next() {
                Some(n) => match n.parse() {
                    Ok(n) => n,
                    Err(e) => {
                        return Err((
                            axum::http::StatusCode::BAD_REQUEST,
                            axum::Json(serde_json::json!({ "error": e.to_string() })),
                        ))
                    }
                },
                None => {
                    return Err((
                        axum::http::StatusCode::BAD_REQUEST,
                        axum::Json(serde_json::json!({ "error": "Must supply the number of entries" })),
                    ))
                }
            }
        }
    };

    let build_fields = fields.iter().map(|f| {
        let field_ident = &f.ident;
        quote! {
            #field_ident: #get_next_and_parse,
        }
    });

    let expanded = quote! {

    pub struct #collection_ident(pub Vec<#name>);

    #[axum::async_trait]
    impl<B> axum::extract::FromRequest<B> for #collection_ident
    where
        B: axum::body::HttpBody + Send,
        B::Data: Send,
        B::Error: Into<axum::BoxError>,
    {
        type Rejection = (axum::http::StatusCode, axum::Json<serde_json::Value>);

        async fn from_request(
            req: &mut axum::extract::RequestParts<B>,
        ) -> Result<Self, Self::Rejection> {
            let s = match String::from_request(req).await {
                Ok(value) => value,
                Err(rejection) => {
                    return Err((
                        axum::http::StatusCode::BAD_REQUEST,
                        axum::Json(serde_json::json!({ "error": "Invalid body" })),
                    ))
                }
            };

            let mut iter = s.split_whitespace();

            let count: u32 = #get_next_and_parse;

            let mut vec: std::vec::Vec<#name> = std::vec::Vec::new();

            for _ in 0..count {
                vec.push(#name {
                    #(#build_fields)*
                });
            }

            match iter.next() {
                Some(_) => Err((
                    axum::http::StatusCode::BAD_REQUEST,
                    axum::Json(serde_json::json!({ "error": "Redundant fields" })),
                )),
                None => Ok(#collection_ident(vec)),
            }
        }
    }
    };

    expanded.into()
}
