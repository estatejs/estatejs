FROM nginx:stable-alpine
COPY nginx.conf /etc/nginx/nginx.conf
COPY bin/ /Doc-Site/
EXPOSE 80
ENTRYPOINT ["nginx", "-g", "daemon off;"]